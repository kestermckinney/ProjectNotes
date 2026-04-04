// Copyright (C) 2025, 2026 Paul McKinney
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
    if (!PyErr_Occurred())
        return;

    PyObject *ptype = nullptr, *pvalue = nullptr, *ptraceback = nullptr;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

    QString errorMessage;

    // Use the traceback module to format the full exception including file and line number
    PyObject* tracebackModule = PyImport_ImportModule("traceback");
    if (tracebackModule)
    {
        PyObject* formatFunc = PyObject_GetAttrString(tracebackModule, "format_exception");
        if (formatFunc)
        {
            PyObject* args = PyTuple_Pack(3,
                ptype     ? ptype     : Py_None,
                pvalue    ? pvalue    : Py_None,
                ptraceback ? ptraceback : Py_None);
            if (args)
            {
                PyObject* lines = PyObject_CallObject(formatFunc, args);
                if (lines && PyList_Check(lines))
                {
                    Py_ssize_t len = PyList_Size(lines);
                    for (Py_ssize_t i = 0; i < len; i++)
                    {
                        PyObject* item = PyList_GetItem(lines, i);
                        const char* str = PyUnicode_AsUTF8(item);
                        if (str)
                            errorMessage += QString::fromUtf8(str);
                    }
                }
                Py_XDECREF(lines);
                Py_XDECREF(args);
            }
            Py_XDECREF(formatFunc);
        }
        Py_XDECREF(tracebackModule);
    }

    // Fall back to just the exception value string if traceback formatting failed
    if (errorMessage.isEmpty() && pvalue)
    {
        PyObject* str = PyObject_Str(pvalue);
        if (str)
        {
            const char* cstr = PyUnicode_AsUTF8(str);
            if (cstr)
                errorMessage = QString::fromUtf8(cstr);
            Py_XDECREF(str);
        }
    }

    if (!errorMessage.isEmpty())
        QLog_Error(ERRORLOG, QString("Python error in plugin %1:\n%2").arg(m_modulename, errorMessage));

    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);

    PyErr_Clear();
}

void PythonWorker::checkForMember(const QString& member)
{
    if (PyObject_HasAttrStringWithError(m_PNPluginModule, member.toUtf8().constData()) == 1)
        m_plugin.addMember(member);
}

void PythonWorker::loadModule(const QString& modulepath)
{
    QMutexLocker locker(&m_loadingmutex);
    m_isloading = true;

    PyGILState_STATE gstate = PyGILState_Ensure();

    m_modulepath = modulepath;

    QFileInfo fileinfo(modulepath);

    m_modulename = fileinfo.baseName();

    if (!fileinfo.exists())
    {
        PyGILState_Release(gstate);

        m_isloading = false;
        m_loadwait.wakeAll();
        return;
    }

    m_PluginLocation = QFileInfo(modulepath).absoluteFilePath();

    m_PNPluginModule = PyImport_ImportModule(m_modulename.toUtf8().constData());
    if (!m_PNPluginModule)
    {
        emitError();
        PyGILState_Release(gstate);
        m_isloading = false;
        m_loadwait.wakeAll();
        return;
    }

    findImportedModules(m_PluginLocation);

    m_plugin.setName(getPythonVariable("pluginname"));
    if (m_plugin.name().isEmpty())
    {
        QLog_Info(CONSOLELOG, QString("plugin name for module %1 can not be empty.").arg(m_modulename));
        PyGILState_Release(gstate);
        m_isloading = false;
        m_loadwait.wakeAll();
        return;
    }

    m_plugin.setDescription(getPythonVariable("plugindescription"));
    if (m_plugin.description().isEmpty())
    {
        QLog_Info(CONSOLELOG, QString("plugin description for module %1 can not be empty.").arg(m_modulename));
        PyGILState_Release(gstate);
        m_isloading = false;
        m_loadwait.wakeAll();
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

void PythonWorker::sendMethodXml(const QString& method, const QString& xml, const QString& parameter)
{
    // if loading or unloading wait to try the call
    if (m_isloading)
    {
        m_loadingmutex.lock();
        if (!m_loadwait.wait(&m_loadingmutex, 10000))
        {
            m_loadingmutex.unlock();
            return;
        }
        m_loadingmutex.unlock();
    }

    if (!m_isloaded)
    {
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    char* result;
    QString val;

    PyObject* pymethod = PyObject_GetAttrString(m_PNPluginModule, method.toUtf8().constData());
    if (!pymethod)
    {
        emitError();

        PyGILState_Release(gstate);
        return;
    }

    PyObject* pargs = Py_BuildValue("(ss)", xml.toUtf8().data(), parameter.toUtf8().data());
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

void PythonWorker::sendMethod(const QString& method, const QString& parameter)
{
    // if loading or unloading wait to try the call
    if (m_isloading)
    {
        m_loadingmutex.lock();
        if (!m_loadwait.wait(&m_loadingmutex, 10000))
        {
            m_loadingmutex.unlock();
            return;
        }
        m_loadingmutex.unlock();
    }

    if (!m_isloaded)
    {
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    char* result;
    QString val;

    PyObject* pymethod = PyObject_GetAttrString(m_PNPluginModule, method.toUtf8().constData());
    if (!pymethod)
    {
        emitError();

        PyGILState_Release(gstate);
        return;
    }

    PyObject* pargs = Py_BuildValue("(s)", parameter.toUtf8().data());
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

int PythonWorker::setPythonVariable(const QString& variablename, const QString& value)
{
    const QByteArray varBytes = variablename.toUtf8();
    PyObject* objectval = PyUnicode_FromString(value.toUtf8().constData());

    int r = PyObject_SetAttrString(m_PNPluginModule, varBytes.constData(), objectval);
    if (r == -1)
    {
        emitError();
        Py_XDECREF(objectval);
        return -1;
    }

    Py_XDECREF(objectval);

    return r;
}

QString PythonWorker::getPythonVariable(const QString& variablename)
{
    QString val;

    const QByteArray varBytes = variablename.toUtf8();
    if (PyObject_HasAttrStringWithError(m_PNPluginModule, varBytes.constData()) != 1)
    {
        return val;
    }

    PyObject* attr = PyObject_GetAttrString(m_PNPluginModule, varBytes.constData());
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

QStringList PythonWorker::getPythonStringList(const QString& variablename)
{
    QStringList val;

    const QByteArray varBytes = variablename.toUtf8();
    if (PyObject_HasAttrStringWithError(m_PNPluginModule, varBytes.constData()) != 1)
    {
        return val;
    }

    PyObject* attr = PyObject_GetAttrString(m_PNPluginModule, varBytes.constData());
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
    val.reserve(static_cast<int>(sz));

    for (Py_ssize_t i = 0; i < sz; i++)
    {
        PyObject* item = PyList_GetItem(attr, i);
        if (!item)
        {
            emitError();
            return val;
        }

        const char* str = PyUnicode_AsUTF8(item);
        val.append(QString::fromUtf8(str));
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
