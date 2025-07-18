#include "pythonworker.h"
#include <QFileInfo>
#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>

// TODO: remove
#ifndef Q_OS_WIN
#if PY_VERSION_HEX < 0x030C0000 // Python 3.10
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
        PyObject *ptype = nullptr, *pvalue = nullptr, *ptraceback = nullptr;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

        // Convert to string for printing
        PyObject* ptypeStr = PyObject_Str(ptype);
        PyObject* pvalueStr = PyObject_Str(pvalue);

        QString errorMsg = QString("Module: %1 ").arg(m_modulename);
        QLog_Info(CONSOLELOG, errorMsg);
        qDebug() << errorMsg;

        if (ptypeStr)
        {
            errorMsg = QString("Error Type: %1" ).arg(PyUnicode_AsUTF8(ptypeStr));
            QLog_Info(CONSOLELOG, errorMsg);
            qDebug() << errorMsg;
            Py_DECREF(ptypeStr);
        }

        if (pvalueStr)
        {
            errorMsg = QString("Error Value: %1 ").arg(PyUnicode_AsUTF8(pvalueStr));
            QLog_Info(CONSOLELOG, errorMsg);
            qDebug() << errorMsg;
            Py_DECREF(pvalueStr);
        }

        if (ptraceback)
        {
            PyObject *traceback_module = PyImport_ImportModule("traceback");
            if (traceback_module)
            {
                // Get format_exception function
                PyObject *formatExc = PyObject_GetAttrString(traceback_module, "format_tb");

                if (formatExc) // && PyCallable_Check(formatExc))
                {
                    // Call format_exception to get the stack trace
                    PyObject *args = PyTuple_Pack(3, ptype, pvalue ? pvalue : Py_None, ptraceback ? ptraceback : Py_None);
                    PyObject *traceList = PyObject_CallObject(formatExc, args);

                    if (traceList && PyList_Check(traceList))
                    {
                        // Convert the stack trace to a QString
                        for (Py_ssize_t i = 0; i < PyList_Size(traceList); ++i)
                        {
                            PyObject *line = PyList_GetItem(traceList, i);
                            if (PyUnicode_Check(line))
                            {
                                QString errorMsg = QString::fromUtf8(PyUnicode_AsUTF8(line)).trimmed();
                                QLog_Info(CONSOLELOG, errorMsg);
                                qDebug() << errorMsg;

                            }
                        }
                    }

                    Py_XDECREF(args);
                    Py_XDECREF(traceList);
                    Py_XDECREF(formatExc);
                }
                Py_XDECREF(traceback_module);

            }

            // Clean up exception objects
            // Py_XDECREF(ptype);
            // Py_XDECREF(pvalue);
            Py_XDECREF(ptraceback);
        }

        // Clean up error state
        PyErr_Clear();
    }
}

void PythonWorker::checkForMember(const QString& t_member)
{
    if (PyObject_HasAttrStringWithError(m_PNPluginModule, t_member.toUtf8().constData()) == 1)
        m_plugin.addMember(t_member);
}

void PythonWorker::loadModule(const QString& t_modulepath)
{
    QMutexLocker locker(&m_loadingmutex);
    m_isloading = true;

    PyGILState_STATE gstate = PyGILState_Ensure();

    m_modulepath = t_modulepath;

    QFileInfo fileinfo(t_modulepath);

    m_modulename = fileinfo.baseName();

    if (!fileinfo.exists())
    {
        PyGILState_Release(gstate);

        QLog_Info(APPLOG, QString("'%1' no longer exists. Load cancelled.").arg(m_modulename));
        return;
    }

    m_PluginLocation = QFileInfo(t_modulepath).absoluteFilePath();

    m_PNPluginModule = PyImport_ImportModule(m_modulename.toUtf8().constData());
    if (!m_PNPluginModule)
    {
        emitError();
        PyGILState_Release(gstate);
        return;
    }

    findImportedModules(m_PluginLocation);

    m_plugin.setName(getPythonVariable("pluginname"));
    if (m_plugin.name().isEmpty())
    {
        QLog_Info(CONSOLELOG, QString("plugin name for module %1 can not be empty.").arg(m_modulename));
        PyGILState_Release(gstate);
        return;
    }

    m_plugin.setDescription(getPythonVariable("plugindescription"));
    if (m_plugin.description().isEmpty())
    {
        QLog_Info(CONSOLELOG, QString("plugin description for module %1 can not be empty.").arg(m_modulename));
        PyGILState_Release(gstate);
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
        if (dictArray && PyList_Check(dictArray))
        {
            Py_ssize_t len = PyList_Size(dictArray);


            for (Py_ssize_t i = 0; i < len; ++i)
            {
                PyObject* subdict = PyList_GetItem(dictArray, i);

                const char* menutitle = nullptr;
                const char* functionname = nullptr;
                const char* tablefilter = nullptr;
                const char* submenu = nullptr;
                const char* dataexport = nullptr;
                const char* parameter = nullptr;

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
                    value = PyDict_GetItemString(subdict, "parameter");
                    if (value)
                        parameter = PyUnicode_AsUTF8(value);

                    m_plugin.addMenu(QString(menutitle),QString(functionname),QString(tablefilter),QString(submenu),QString(dataexport),QString(parameter));
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

    m_timer = new QTimer();
    m_timer->start(1000*60 * m_plugin.timerdelay()); // one minute timer event

    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerUpdate()), Qt::DirectConnection);
    emit loadComplete(m_plugin);

    m_loadwait.wakeAll();

    m_isloading = false;
    m_isloaded = true;

    if (m_plugin.hasMember("event_startup"))
    {
        sendMethod("event_startup", "");
    }

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
        sendMethod("event_shutdown", "");
    }

    m_isloading = true;

    m_timer->stop();

    disconnect(m_timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    delete m_timer;

    m_plugin.clearMenu();

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* sysModules = PyImport_GetModuleDict();
    if (!sysModules)
    {
        emitError();

        PyGILState_Release(gstate);
        return;
    }

    // Remove the module from sys.modules
    if (PyDict_DelItemString(sysModules, m_modulename.toUtf8().constData()) < 0)
    {
        emitError();
    }

    Py_XDECREF(m_PNPluginModule);

    PyGILState_Release(gstate);

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

void PythonWorker::sendMethodXml(const QString& t_method, const QString& t_xml, const QString& t_parameter)
{
    // if loading or unloading wait to try the call
    if (m_isloading)
        if (!m_loadwait.wait(&m_loadingmutex, 10000))
        {
            QLog_Info(APPLOG, QString("Module took to long to load! SendMethodXML cancelled."));
            return;
        }

    if (!m_isloaded)
    {
        QLog_Info(APPLOG, QString("Module is not loaded! SendMethodXml cancelled."));
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    char* result;
    QString val;

    PyObject* pymethod = PyObject_GetAttrString(m_PNPluginModule, t_method.toUtf8().constData());
    if (!pymethod)
    {
        emitError();

        PyGILState_Release(gstate);
        return;
    }

    PyObject* pargs = Py_BuildValue("(ss)", t_xml.toUtf8().data(), t_parameter.toUtf8().data());
    if (!pargs)
    {
        emitError();
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        return;
    }

    PyObject* func = PyObject_CallObject(pymethod, pargs);
    if (!func)
    {
        emitError();
        Py_XDECREF(pargs);
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        return;
    }

    Py_XDECREF(pargs);

    if (!PyArg_Parse(func, "s", &result))                /* convert to C */
    {
        emitError();
        Py_XDECREF(func);
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        return;
    }

    val = QString::fromUtf8(result);

    Py_XDECREF(func);
    Py_XDECREF(pymethod);

    PyGILState_Release(gstate);

    emit returnXml(val);
}

void PythonWorker::sendMethod(const QString& t_method, const QString& t_parameter)
{
    // if loading or unloading wait to try the call
    if (m_isloading)
        if (!m_loadwait.wait(&m_loadingmutex, 10000))
        {
            QLog_Info(APPLOG, QString("Module took to long to load! SendMethod cancelled."));
            return;
        }

    if (!m_isloaded)
    {
        QLog_Info(APPLOG, QString("Module is not loaded! SendMethod cancelled."));
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    char* result;
    QString val;

    PyObject* pymethod = PyObject_GetAttrString(m_PNPluginModule, t_method.toUtf8().constData());
    if (!pymethod)
    {
        emitError();

        PyGILState_Release(gstate);
        return;
    }

    PyObject* pargs = Py_BuildValue("(s)", t_parameter.toUtf8().data());
    if (!pargs)
    {
        emitError();
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        return;
    }

    PyObject* func = PyObject_CallObject(pymethod, pargs);
    if (!func)
    {
        emitError();
        Py_XDECREF(pargs);
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);
        return;
    }

    Py_XDECREF(pargs);

    if (!PyArg_Parse(func, "s", &result))                /* convert to C */
    {
        emitError();
        Py_XDECREF(func);
        Py_XDECREF(pymethod);

        PyGILState_Release(gstate);

        return;
    }

    val = QString(result);

    Py_XDECREF(func);
    Py_XDECREF(pymethod);

    PyGILState_Release(gstate);

    emit returnXml(val);
}

void PythonWorker::timerUpdate()
{
    // call the menu plugin with the data structure
    if (m_plugin.hasMember("event_timer"))
    {
        sendMethod("event_timer", "");
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
    QString val;

    if (PyObject_HasAttrStringWithError(m_PNPluginModule, t_variablename.toStdString().c_str()) != 1)
    {
        return val;
    }

    PyObject* attr = PyObject_GetAttrString(m_PNPluginModule, t_variablename.toStdString().c_str());
    if (!attr)
    {
        emitError();
        return val;
    }

    const char* str = PyUnicode_AsUTF8(attr);
    if (!str)
    {
        emitError();
        Py_XDECREF(attr);
        return val;
    }

    val = QString::fromUtf8(str);

    Py_XDECREF(attr);

    return val;
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
        return val;
    }

    if (!PyList_Check(attr))
    {
        emitError();
        Py_XDECREF(attr);
        return val;
    }

    Py_ssize_t sz = PyList_Size(attr);

    for (long i = 0; i < sz; i++)
    {
        PyObject* item = PyList_GetItem(attr, i);
        if (!item)
        {
            emitError();
            return val;
        }

        const char* str = PyUnicode_AsUTF8(item);
        QString memval = QString::fromUtf8(str);

        val.append(memval);
    }

    Py_XDECREF(attr);

    return val;
}

void PythonWorker::findImportedModules(const QString& pythonFilePath)
{
    // Open the Python script file
    QFile file(pythonFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return; // Return if file cannot be opened
    }

    // Get the directory of the Python script
    QDir scriptDir(QFileInfo(pythonFilePath).absolutePath());

    // Get the application directory for .ui files
    QDir appDir(QCoreApplication::applicationDirPath());

    QTextStream in(&file);
    // Regular expression to match import statements
    QRegularExpression importRegex(R"(^\s*(?:import|from)\s+([\w.]+)(?:\s+import\s+[\w, ]+)?\s*$)");

    // Regular expression for loadUi calls
    QRegularExpression loadUiRegex(R"(\bloadUi\s*\(\s*['"]([^'"]+\.ui)['"]\s*(?:,\s*[^\)]+)?\s*\))");

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#'))
        {
            continue;
        }

        // Match import statements
        QRegularExpressionMatch match = importRegex.match(line);
        if (match.hasMatch())
        {
            QString moduleName = match.captured(1);

            // Split module name by dots for submodules
            QStringList moduleParts = moduleName.split('.');
            QString modulePath = moduleParts.join('/');

            // Check for both .py and .pyc files, and directory modules (__init__.py)
            QStringList possiblePaths = {
                scriptDir.absoluteFilePath(modulePath + ".py"),
                scriptDir.absoluteFilePath(modulePath + ".pyc"),
                scriptDir.absoluteFilePath(modulePath + "/__init__.py")
            };

            // Add valid paths to the result
            for (const QString& path : possiblePaths)
            {
                if (QFile::exists(path))
                {
                    m_plugin.addImport(QDir::cleanPath(path));
                }
            }
        }

        // Match loadUi calls
        QRegularExpressionMatch loadUiMatch = loadUiRegex.match(line);
        if (loadUiMatch.hasMatch())
        {
            QString uiFileName = loadUiMatch.captured(1);
            QString uiFilePath = appDir.absoluteFilePath(uiFileName);

            // Add valid .ui file path to the result
            if (QFile::exists(uiFilePath))
            {
                m_plugin.addImport(QDir::cleanPath(uiFilePath));
            }
        }
    }

    file.close();
}
