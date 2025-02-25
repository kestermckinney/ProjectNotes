#include "mainwindow.h"
#include "pluginmanager.h"

#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
#include <QDomDocument>
#include <frameobject.h>

#include "QLogger.h"
#include "QLoggerWriter.h"
#include "pndatabaseobjects.h"

using namespace QLogger;


struct Stdout
{
    PyObject_HEAD
    stdout_write_type write;
};

PyObject* Stdout_write(PyObject* self, PyObject* args)
{
    std::size_t written(0);
    Stdout* selfimpl = reinterpret_cast<Stdout*>(self);
    if (selfimpl->write)
    {
        char* data;
        if (!PyArg_ParseTuple(args, "s", &data))
            return 0;

        std::string str(data);
        selfimpl->write(str);
        written = str.size();
    }
    return PyLong_FromSize_t(written);
}

PyObject* Stdout_flush(PyObject* self, PyObject* args)
{
    Q_UNUSED(self);
    Q_UNUSED(args);

    // no-op
    return Py_BuildValue("");
}

static PyObject* getCallerModuleName()
{
    // Get the current frame
    PyFrameObject* frame = PyEval_GetFrame();

    if (frame) {
        // Get the global namespace dictionary
        PyObject* globals = PyFrame_GetGlobals(frame);

        if (globals) {
            // Get the module name from the globals dictionary
            PyObject* moduleName = PyDict_GetItemString(globals, "__name__");

            if (moduleName) {
                return moduleName;
            }
        }
    }

    return nullptr;
}

static PyObject* update_data(PyObject* self, PyObject* args)
{
    Q_UNUSED(self);

    const char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
    {
        return PyBool_FromLong(0);
    }

    PyObject* pycaller = getCallerModuleName();
    QString caller = QString::fromUtf8(PyUnicode_AsUTF8(pycaller));

    QDomDocument xmldoc;
    QByteArray ba(input);

    xmldoc.setContent(ba);

    PNDatabaseObjects dbo;
    dbo.openDatabase(global_DBObjects.getDatabaseFile(), caller, false);
    int result = dbo.importXMLDoc(xmldoc);
    dbo.closeDatabase();

    Py_XDECREF(pycaller);

    return PyBool_FromLong(result);
}


static PyObject* get_data(PyObject* self, PyObject* args)
{
    Q_UNUSED(self);

    const char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
    {
        Py_RETURN_NONE;
    }

    PyObject* pycaller = getCallerModuleName();
    QString caller = QString::fromUtf8(PyUnicode_AsUTF8(pycaller));

    QDomDocument xmldoc;
    QByteArray ba(input);

    xmldoc.setContent(ba);

    PNDatabaseObjects dbo;
    dbo.openDatabase(global_DBObjects.getDatabaseFile(), caller, false);
    QList<PNSqlQueryModel*>* models = dbo.getData(xmldoc);

    if (!models)
    {
        dbo.closeDatabase();
        Py_RETURN_NONE;
    }

    QDomDocument* returnxmldoc = dbo.createXMLExportDoc(models);

    qDeleteAll(*models);
    models->clear();
    delete models;

    if (!returnxmldoc)
    {
        dbo.closeDatabase();
        Py_RETURN_NONE;
    }

    QString xmlstring = returnxmldoc->toString();

    dbo.closeDatabase();

    Py_XDECREF(pycaller);

    delete returnxmldoc;

    PyObject* pyString = Py_BuildValue("s", xmlstring.toUtf8().constData());
    return pyString;
}

static PyObject* force_reload(PyObject* self, PyObject* args)
{
    Q_UNUSED(self);

    const char* modulename;
    if (!PyArg_ParseTuple(args, "s", &modulename))
    {
        Py_RETURN_NONE;
    }

    // put the reload request on the queue
    MainWindow::getPluginManager()->forceReload(QString(modulename));

    Py_RETURN_NONE;
}



PyMethodDef Stdout_methods[] =
{
    {"write", Stdout_write, METH_VARARGS, "sys.stdout.write"},
    {"flush", Stdout_flush, METH_VARARGS, "sys.stdout.write"},
    {0, 0, 0, 0} // sentinel
};

PyMethodDef data_methods[] =
    {
        {"update_data", update_data, METH_VARARGS, "projectnotes.update_data"},
        {"get_data", get_data, METH_VARARGS, "projectnotes.get_data"},
        {"force_reload", force_reload, METH_VARARGS, "projectnotes.force_reload"},
        {0, 0, 0, 0} // sentinel
};

PyTypeObject StdoutType =
{
    PyVarObject_HEAD_INIT(0, 0)
    "projectnotes.StdoutType",     /* tp_name */
    sizeof(Stdout),       /* tp_basicsize */
    0,                    /* tp_itemsize */
    0,                    /* tp_dealloc */
    0,                    /* tp_print */
    0,                    /* tp_getattr */
    0,                    /* tp_setattr */
    0,                    /* tp_reserved */
    0,                    /* tp_repr */
    0,                    /* tp_as_number */
    0,                    /* tp_as_sequence */
    0,                    /* tp_as_mapping */
    0,                    /* tp_hash  */
    0,                    /* tp_call */
    0,                    /* tp_str */
    0,                    /* tp_getattro */
    0,                    /* tp_setattro */
    0,                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,   /* tp_flags */
    "projectnotes.Stdout objects", /* tp_doc */
    0,                    /* tp_traverse */
    0,                    /* tp_clear */
    0,                    /* tp_richcompare */
    0,                    /* tp_weaklistoffset */
    0,                    /* tp_iter */
    0,                    /* tp_iternext */
    Stdout_methods,       /* tp_methods */
    0,                    /* tp_members */
    0,                    /* tp_getset */
    0,                    /* tp_base */
    0,                    /* tp_dict */
    0,                    /* tp_descr_get */
    0,                    /* tp_descr_set */
    0,                    /* tp_dictoffset */
    0,                    /* tp_init */
    0,                    /* tp_alloc */
    0,                    /* tp_new */
    0,                    /* tp_free */
    0,                    /* tp_is_gc */
    0,                    /* tp_bases */
    0,                    /* tp_mro */
    0,                    /* tp_cache */
    0,                    /* tp_subclasses */
    0,                    /* tp_weaklist */
    0,                    /* tp_del */
    0,                    /* tp_version_tag */
    0,                    /* tp_finalize */
    0,                    /* tp_vectorcall */
    0,                    /* tp_watched */
};

PyModuleDef embmodule =
    {
        PyModuleDef_HEAD_INIT,
        "projectnotes",
        "Project Notes module callable from embeded Python",
        -1,
        data_methods, 0, 0, 0, 0,
};

// Internal state
PyObject* g_stdout;
PyObject* g_stdout_saved;

PyObject* g_stderr;
PyObject* g_stderr_saved;

PyMODINIT_FUNC PyInit_embeddedconsole(void)
{
    g_stdout = 0;
    g_stdout_saved = 0;

    g_stderr = 0;
    g_stderr_saved = 0;

    StdoutType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&StdoutType) < 0)
        return 0;

    PyObject* m = PyModule_Create(&embmodule);

    if (m)
    {
        Py_INCREF(&StdoutType);
        PyModule_AddObject(m, "Stdout", reinterpret_cast<PyObject*>(&StdoutType));
    }

    return m;
}

void set_stdout(stdout_write_type write)
{
    if (!g_stdout)
    {
        g_stdout_saved = PySys_GetObject("stdout"); // borrowed
        g_stdout = StdoutType.tp_new(&StdoutType, 0, 0);
    }

    Stdout* impl = reinterpret_cast<Stdout*>(g_stdout);
    impl->write = write;
    PySys_SetObject("stdout", g_stdout);


    if (!g_stderr)
    {
        g_stderr_saved = PySys_GetObject("stderr"); // borrowed
        g_stderr = StdoutType.tp_new(&StdoutType, 0, 0);
    }

    Stdout* impl_err = reinterpret_cast<Stdout*>(g_stderr);
    impl_err->write = write;

    PySys_SetObject("stderr", g_stderr);
}

void reset_stdout()
{
    if (g_stdout_saved)
        PySys_SetObject("stdout", g_stdout_saved);

    Py_XDECREF(g_stdout);
    g_stdout = 0;

    if (g_stderr_saved)
        PySys_SetObject("stderr", g_stderr_saved);

    Py_XDECREF(g_stderr);
    g_stderr = 0;
}

PluginManager::PluginManager(QObject *parent)
    : QObject{parent}
{

    m_pluginspath = QCoreApplication::applicationDirPath() + "/plugins/";
    m_threadspath = QCoreApplication::applicationDirPath() + "/threads/";

    QString pythonpath =  QCoreApplication::applicationDirPath();
    QString pluginspath = QString("sys.path.append(\"%1\")").arg(m_pluginspath);
    QString threadspath = QString("sys.path.append(\"%1\")").arg(m_threadspath);

    QString pythonzip = QString("sys.path.append(\"%1\")").arg(pythonpath + "/python313.zip");
    QString sitepackages = QString("sys.path.append(\"%1\")").arg(pythonpath + "/site-packages");
    QString win32path = QString("sys.path.append(\"%1\")").arg(pythonpath + "/site-packages/win32");
    QString win32lib = QString("sys.path.append(\"%1\")").arg(pythonpath + "/site-packages/win32/lib");
    QString pythonwin = QString("sys.path.append(\"%1\")").arg(pythonpath + "/site-packages/Pythonwin");

    PyStatus status;
    PyConfig config;

    PyConfig_InitPythonConfig(&config);

    status = PyConfig_SetString(&config, &config.program_name, L"Project Notes");
    if (PyStatus_Exception(status))
    {
        PyConfig_Clear(&config);
        QLog_Info(CONSOLELOG,QString("Embeded Python failed to start."));
        return;
    }

    PyImport_AppendInittab("projectnotes", PyInit_embeddedconsole);

    Py_Initialize();

    PyImport_ImportModule("projectnotes");

    stdout_write_type outwrite = [this] (std::string s)
    {
        std::string text = s;

        // ignore double return console messages
        if (text == "\n\n")
            return;

        if (text == "\n")
            return;

        QLog_Info(CONSOLELOG, QString::fromStdString(text));
    };

    set_stdout(outwrite);

    // load PNPlugin modules
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(pluginspath.toUtf8().constData());
    PyRun_SimpleString(threadspath.toUtf8().constData());
    PyRun_SimpleString(pythonzip.toUtf8().constData());
    PyRun_SimpleString(sitepackages.toUtf8().constData());
    PyRun_SimpleString(win32path.toUtf8().constData());
    PyRun_SimpleString(win32lib.toUtf8().constData());
    PyRun_SimpleString(pythonwin.toUtf8().constData());
// #ifdef QT_DEBUG
// TODO: remove
//     PyRun_SimpleString(QString("import os").toUtf8().constData());
//     PyRun_SimpleString(QString("print(os.path.dirname(sys.executable))").toUtf8().constData());
//     PyRun_SimpleString(QString("sys.path.append('c:/Program Files/Python313/Lib/site-packages')").toUtf8().constData());
//     PyRun_SimpleString(QString("sys.path.append('c:/Program Files/Python313/Lib/site-packages/win32')").toUtf8().constData());
//     PyRun_SimpleString(QString("sys.path.append('c:/Program Files/Python313/Lib/site-packages/win32/lib')").toUtf8().constData());
//     PyRun_SimpleString(QString("sys.path.append('c:/Program Files/Python313/Lib/site-packages/pythonwin')").toUtf8().constData());
//     PyRun_SimpleString(QString("sys.path.append('c:/Program Files/Python313/Lib/site-packages/pywin32_system32')").toUtf8().constData());
// #endif

    QLog_Info(CONSOLELOG, QString("Embedded Python Version %1").arg(Py_GetVersion()));

    m_pythreadstate = PyEval_SaveThread();

    // Set up file watcher
    m_fileWatcher = new QFileSystemWatcher(this);
    m_fileWatcher->addPath(m_pluginspath);
    m_fileWatcher->addPath(m_threadspath);

    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &PluginManager::onFileChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &PluginManager::onFolderChanged);
    connect(this, &PluginManager::pluginForceReload, this, &PluginManager::onForceReload);

    loadPluginFiles(m_pluginspath, false);
    loadPluginFiles(m_threadspath, true);
}

void PluginManager::loadPluginFiles(const QString& t_path, bool t_isthread)
{
    QDirIterator fileit(t_path, {"*.py"}, QDir::Files);

    // iterate through PNPlugins folder and load py files
    while (fileit.hasNext())
    {
        fileit.next();

        bool notfound = true;
        QString filePath = fileit.fileInfo().absoluteFilePath();

        for (auto it = m_pluginlist.begin(); it != m_pluginlist.end(); ++it)
        {
            if ((*it)->modulepath().compare(filePath) == 0)
            {
                notfound = false;
            }
        }

        if (notfound)
        {
            Plugin* module = new Plugin(this, t_isthread);
            m_pluginlist.append(module);

            m_fileWatcher->addPath(filePath);

            connect(module, &Plugin::moduleLoaded, this, &PluginManager::pluginLoaded);
            connect(module, &Plugin::moduleUnloaded, this, &PluginManager::pluginUnLoaded);

            module->loadPlugin(filePath);
        }
    }
}

// close Plugin engine
PluginManager::~PluginManager()
{
    disconnect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &PluginManager::onFileChanged);
    disconnect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &PluginManager::onFolderChanged);
    disconnect(this, &PluginManager::pluginForceReload, this, &PluginManager::onForceReload);

    for ( Plugin* p : m_pluginlist)
    {
        disconnect(p, &Plugin::moduleLoaded, this, &PluginManager::pluginLoaded);
        disconnect(p, &Plugin::moduleUnloaded, this, &PluginManager::pluginUnLoaded);

        delete p;
    }

    m_pluginlist.clear();

    PyEval_RestoreThread(m_pythreadstate);

    reset_stdout();

    Py_FinalizeEx();
}

void PluginManager::unloadAll()
{
    for ( Plugin* p : m_pluginlist)
    {
        p->unloadPlugin();
    }
}

void PluginManager::onUnloadComplete(const QString &t_modulepath)
{
    QLog_Info(CONSOLELOG, QString("Module '%1' unloaded.").arg(t_modulepath));

    QFileInfo file(t_modulepath);

    if (!file.exists())
    {
        // file is gone, close out the thread
        for (auto it = m_pluginlist.begin(); it != m_pluginlist.end();)
        {
            if ((*it)->modulepath().compare(t_modulepath) == 0)
            {
                m_fileWatcher->removePath(t_modulepath);

                delete *it;
                it = m_pluginlist.erase(it);
                QLog_Info(CONSOLELOG, QString("Module '%1' no longer exists. Thread stopped.").arg(t_modulepath));
            }
            else
            {
                ++it;
            }
        }
    }

    emit pluginUnLoaded(t_modulepath);
}

void PluginManager::onFileChanged(const QString &t_filepath)
{
    QLog_Info(CONSOLELOG, QString("Module file '%1' changed.").arg(t_filepath));

    QFileInfo file(t_filepath);

    for (auto it = m_pluginlist.begin(); it != m_pluginlist.end(); ++it)
    {
        if ((*it)->modulepath().compare(t_filepath, Qt::CaseInsensitive) == 0)
        {
            (*it)->reloadPlugin();
            QLog_Info(CONSOLELOG, QString("Module '%1' reload requested.").arg(t_filepath));
            return;
        }
    }

    QLog_Info(CONSOLELOG, QString("Module file '%1' was not found. Module reload failed.").arg(t_filepath));
}

void PluginManager::onForceReload(const QString &t_module)
{
    QString basemodule = QFileInfo(t_module).baseName();

    for (auto it = m_pluginlist.begin(); it != m_pluginlist.end(); ++it)
    {
        QString checkmodule = QFileInfo((*it)->modulepath()).baseName();

        if (checkmodule.compare(basemodule, Qt::CaseInsensitive) == 0)
        {
            (*it)->reloadPlugin();
            QLog_Info(CONSOLELOG, QString("Module '%1' force reload requested.").arg(t_module));
            return;
        }
    }

    QLog_Info(CONSOLELOG, QString("Module '%1' was not found. Module reload failed.").arg(t_module));
}

void PluginManager::onFolderChanged(const QString &folderPath)
{
    loadPluginFiles(m_pluginspath, false);
    loadPluginFiles(m_threadspath, true);
}

void PluginManager::onLoadComplete(const QString& t_modulepath)
{
    emit pluginLoaded(t_modulepath);
}

void PluginManager::forceReload(const QString& t_module)
{
    emit pluginForceReload(t_module);
}
