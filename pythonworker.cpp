#include "pythonworker.h"
#include <QFileInfo>

#if PY_VERSION_HEX < 0x03130000 // Python 3.10
static int PyObject_HasAttrStringWithError(PyObject *obj, const char *attr_name) {
    if (!obj || !attr_name) {
        PyErr_SetString(PyExc_TypeError, "null argument to PyObject_HasAttrStringWithError");
        return -1;
    }
    int result = PyObject_HasAttrString(obj, attr_name);
    if (result == 0) {
        return 0;  // Attribute does not exist
    } else if (result == -1) {
        PyErr_Clear();  // Clear error as PyObject_HasAttrString does not set it properly
    }
    return result;
}
#endif

PythonWorker::PythonWorker(QObject *parent)
    : QObject{parent}
{

}


void PythonWorker::emitError()
{
    // Function to retrieve and print Python error information
    if (PyErr_Occurred())
    {
        // Fetch error information
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);

        // Convert to string for printing
        PyObject* ptypeStr = PyObject_Str(ptype);
        PyObject* pvalueStr = PyObject_Str(pvalue);
        PyObject* ptracebackStr = PyObject_Str(ptraceback);

        QString errorMsg;

        if (ptypeStr)
        {
            errorMsg = QString("Error Type: %1" ).arg(PyUnicode_AsUTF8(ptypeStr));
            Py_DECREF(ptypeStr);
        }

        if (pvalueStr)
        {
            errorMsg += QString("Error Value: %1 ").arg(PyUnicode_AsUTF8(pvalueStr));
            Py_DECREF(pvalueStr);
        }

        if (ptracebackStr)
        {
            errorMsg += QString("Traceback: %1 ").arg(PyUnicode_AsUTF8(ptracebackStr));
            Py_DECREF(ptracebackStr);
        }

        // Clean up error state
        PyErr_Clear();

        QLog_Info(CONSOLEMOD, errorMsg);
    }
}

void PythonWorker::checkForMember(const QString& t_member)
{
    if (PyObject_HasAttrStringWithError(m_PNPluginModule, t_member.toUtf8().constData()) == 1)
        m_plugin.addMember(t_member);
}

void PythonWorker::loadModule(const QString& t_modulepath)
{
    // QLog_Debug(PLUGINSMOD, QString("Entered load slot for '%1").arg(t_modulepath)); TODO Remove

    QMutexLocker locker(&m_loadingmutex);
    m_isloading = true;

    PyGILState_STATE gstate = PyGILState_Ensure();
    // QLog_Debug(PLUGINSMOD, QString("Called GIL Ensure State in '%1'.").arg(Q_FUNC_INFO));

    m_modulepath = t_modulepath;

    QFileInfo fileinfo(t_modulepath);

    m_modulename = fileinfo.baseName();

    if (!fileinfo.exists())
    {
        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO));
        QLog_Debug(PLUGINSMOD, QString("'%1' no longer exists. Load cancelled.").arg(m_modulename));
        return;
    }

    m_PluginLocation = QFileInfo(t_modulepath).absoluteFilePath();

    m_PNPluginModule = PyImport_ImportModule(m_modulename.toUtf8().constData());
    if (!m_PNPluginModule)
    {
        emitError();
        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove
        return;
    }

    m_plugin.setName(getPythonVariable("pluginname"));
    if (m_plugin.name().isEmpty())
    {
        QLog_Info(CONSOLEMOD, QString("plugin name for module %1 can not be empty.").arg(m_modulename));
        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove
        return;
    }

    m_plugin.setDescription(getPythonVariable("plugindescription"));
    if (m_plugin.description().isEmpty())
    {
        QLog_Info(CONSOLEMOD, QString("plugin description for module %1 can not be empty.").arg(m_modulename));
        PyGILState_Release(gstate);
        //QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove
        return;
    }

    QString dlay = getPythonVariable("plugintimerdelay");

    if (dlay.isEmpty())
        m_plugin.setTimerDelay(1);
    else
        m_plugin.setTimerDelay(dlay.toInt());

    if (PyObject_HasAttrStringWithError(m_PNPluginModule, "pluginmenus") == 1)
    {
        PyObject* dictArray = PyObject_GetAttrString(m_PNPluginModule, "pluginmenus");
        if (dictArray)
        {
            //PyObject* key, *subdict;  TODO Remvoe
            //Py_ssize_t pos = 0;

            //while (PyDict_Next(dictArray, &pos, &key, &subdict))
            Py_ssize_t len = PyList_Size(dictArray);
            for (Py_ssize_t i = 0; i < len; ++i)
            {
                PyObject* subdict = PyList_GetItem(dictArray, i);

                const char* menutitle = nullptr;
                const char* functionname = nullptr;
                const char* tablefilter = nullptr;
                const char* submenu = nullptr;
                const char* dataexport = nullptr;

                // Check if the value is another dictionary
                if (PyDict_Check(subdict))
                {
                    PyObject* value = PyDict_GetItemString(subdict, "menutitle");
                    if (value)
                        menutitle = PyUnicode_AsUTF8(value);
                    value = PyDict_GetItemString(subdict, "function");
                    if (value)
                        functionname = PyUnicode_AsUTF8(value);
                    value = PyDict_GetItemString(subdict, "tablefilter");
                    if (value)
                        tablefilter = PyUnicode_AsUTF8(value);
                    value = PyDict_GetItemString(subdict, "submenu");
                    if (value)
                        submenu = PyUnicode_AsUTF8(value);
                    value = PyDict_GetItemString(subdict, "dataexport");
                    if (value)
                        dataexport = PyUnicode_AsUTF8(value);

                    m_plugin.addMenu(QString(menutitle),QString(functionname),QString(tablefilter),QString(submenu),QString(dataexport));
                    m_plugin.addMember(QString(functionname));
                }
            }
        }

        Py_XDECREF(dictArray);
    }

    checkForMember("event_startup");
    checkForMember("event_shutdown");
    checkForMember("event_timer");

    PyGILState_Release(gstate);
    // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove

    //QLog_Debug(PLUGINSMOD, QString("Load Thread is %1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0')));TODO Remove

    m_timer = new QTimer();
    m_timer->start(1000*60 * m_plugin.timerdelay()); // one minute timer event

    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerUpdate()), Qt::DirectConnection);
    emit loadComplete(m_plugin);

    m_loadwait.wakeAll();

    m_isloading = false;
    m_isloaded = true;

    if (m_plugin.hasMember("event_startup"))
    {
        sendMethod("event_startup");
    }

    // QLog_Debug(PLUGINSMOD, "Load Complete."); TODO Remove
}

void PythonWorker::unloadModule()
{
    QMutexLocker locker(&m_loadingmutex);

    if (!m_isloaded)
    {
        return;
    }

    if (m_plugin.hasMember("event_shutdown"))
    {
        sendMethod("event_shutdown");
    }

    m_isloading = true;

    m_timer->stop();

    disconnect(m_timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    delete m_timer;

    m_plugin.clearMenu();

    PyGILState_STATE gstate = PyGILState_Ensure();
    //  TODO RemoveQLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO));

    PyObject* sysModules = PyImport_GetModuleDict();
    if (!sysModules)
    {
        emitError();

        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove
        return;
    }

    // Remove the module from sys.modules
    if (PyDict_DelItemString(sysModules, m_modulename.toUtf8().constData()) < 0)
    {
        emitError();
    }
    // elseTODO Remove
    // {
    //     QLog_Debug(PLUGINSMOD, QString("Module '%1' successfully removed from sys.modules.").arg(m_modulename));
    // }

    // Run garbage collection
    PyObject* gcModule = PyImport_ImportModule("gc");
    if (gcModule)
    {
        PyObject* result = PyObject_CallMethod(gcModule, "collect", nullptr);
        if (result)
        {
            Py_DECREF(result);
        }
        Py_DECREF(gcModule);
    }
    else
    {
        PyErr_Print();
    }

    Py_XDECREF(m_PNPluginModule);

    PyGILState_Release(gstate);
    // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove

    m_isloading = false;
    m_isloaded = false;

    emit unloadComplete();

    m_loadwait.wakeAll();
}

void PythonWorker::reloadModule()
{
    unloadModule();
    loadModule(m_modulepath);
}

void PythonWorker::sendMethodXml(const QString& t_method, const QString& t_xml)
{
    // if loading or unloading wait to try the call
    if (m_isloading)
        if (!m_loadwait.wait(&m_loadingmutex, 10000))
        {
            QLog_Debug(PLUGINSMOD, QString("Module took to long to load! SendMethodXML cancelled."));
            return;
        }

    if (!m_isloaded)
    {
        QLog_Debug(PLUGINSMOD, QString("Module is not loaded! SendMethodXml cancelled."));
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();
    // QLog_Debug(PLUGINSMOD, QString("Called GIL Ensure State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove

    char* result;
    QString val;

    PyObject* pymethod = PyObject_GetAttrString(m_PNPluginModule, t_method.toUtf8().constData());
    if (!pymethod)
    {
        emitError();

        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove
        return;
    }

    PyObject* pargs = Py_BuildValue("(s)", t_xml.toUtf8().data());
    if (!pargs)
    {
        emitError();

        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove
        return;
    }

    PyObject* func = PyObject_CallObject(pymethod, pargs);
    if (!func)
    {
        emitError();
        Py_XDECREF(pargs);
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO));TODO Remove
        return;
    }

    Py_XDECREF(pargs);

    if (!PyArg_Parse(func, "s", &result))                /* convert to C */
    {
        emitError();
        Py_XDECREF(func);
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO));
        return;
    }

    val = QString::fromUtf8(result);

    QLog_Debug(PLUGINSMOD, QString("%1 should return %2").arg(Q_FUNC_INFO).arg(val));

    Py_XDECREF(func);
    Py_XDECREF(pymethod);

    PyGILState_Release(gstate);
    // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO));TODO Remove

    emit returnXml(val);
}

void PythonWorker::sendMethod(const QString& t_method)
{
    // if loading or unloading wait to try the call
    if (m_isloading)
        if (!m_loadwait.wait(&m_loadingmutex, 10000))
        {
            QLog_Debug(PLUGINSMOD, QString("Module took to long to load! SendMethod cancelled."));
            return;
        }

    if (!m_isloaded)
    {
        QLog_Debug(PLUGINSMOD, QString("Module is not loaded! SendMethod cancelled."));
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();
    // QLog_Debug(PLUGINSMOD, QString("Called GIL Ensure State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove

    char* result;
    QString val;

    PyObject* pymethod = PyObject_GetAttrString(m_PNPluginModule, t_method.toUtf8().constData());
    if (!pymethod)
    {
        emitError();

        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Ensure State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove
        return;
    }

    PyObject* func = PyObject_CallObject(pymethod, nullptr);
    if (!func)
    {
        emitError();
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove
        return;
    }

    if (!PyArg_Parse(func, "s", &result))                /* convert to C */
    {
        emitError();
        Py_XDECREF(func);
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO)); TODO Remove

        return;
    }

    val = QString(result);

    Py_XDECREF(func);
    Py_XDECREF(pymethod);

    PyGILState_Release(gstate);
    // QLog_Debug(PLUGINSMOD, QString("Called GIL Release State in '%1'.").arg(Q_FUNC_INFO));TODO Remove

    emit returnXml(val);
}

void PythonWorker::timerUpdate()
{
    // QLog_Debug(PLUGINSMOD, "Timer event called.");TODO Remove

    // call the menu plugin with the data structure
    if (m_plugin.hasMember("event_timer"))
    {
        sendMethod("event_timer");
    }
}

PythonWorker::~PythonWorker()
{

}

int PythonWorker::setPythonVariable(const QString& t_variablename, const QString& t_value)
{
    PyObject* objectval = PyUnicode_FromString(t_value.toStdString().c_str());

    int r = PyObject_SetAttrString(m_PNPluginModule, t_variablename.toStdString().c_str(), objectval);
    if (r == -1)
    {
        emitError();
        return -1;
    }

    Py_XDECREF(objectval);

    return r;
}

QString PythonWorker::getPythonVariable(const QString& t_variablename)
{
    if (PyObject_HasAttrStringWithError(m_PNPluginModule, t_variablename.toStdString().c_str()) != 1)
    {
        return QString();
    }

    PyObject* attr = PyObject_GetAttrString(m_PNPluginModule, t_variablename.toStdString().c_str());
    if (!attr)
    {
        emitError();
        return QString();
    }

    const char* str = PyUnicode_AsUTF8(attr);
    if (!str)
    {
        emitError();
        return QString();
    }

    Py_XDECREF(attr);

    return QString(str);
}

QStringList PythonWorker::getPythonStringList(const QString& t_variablename)
{
    QStringList val;

    if (PyObject_HasAttrStringWithError(m_PNPluginModule, t_variablename.toStdString().c_str()) == 1)
    {
        return val;
    }

    PyObject* attr = PyObject_GetAttrString(m_PNPluginModule, t_variablename.toStdString().c_str());
    if (!attr)
    {
        emitError();
        return QStringList();
    }

    if (!PyList_Check(attr))
    {
        emitError();
        Py_XDECREF(attr);
        return QStringList();
    }

    Py_ssize_t sz = PyList_Size(attr);

    for (long i = 0; i < sz; i++)
    {
        PyObject* item = PyList_GetItem(attr, i);
        if (!item)
        {
            emitError();
            return QStringList();
        }

        const char* str = PyUnicode_AsUTF8(item);
        QString memval = QString::fromUtf8(str);

        val.append(memval);
    }

    Py_XDECREF(attr);

    return val;
}
