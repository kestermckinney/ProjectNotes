#include "mainwindow.h"
#include "pluginmanager.h"

#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
#include <QDomDocument>
#include <frameobject.h>
#include <QApplication>

#include "QLogger.h"
#include "QLoggerWriter.h"
#include "databaseobjects.h"

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

    DatabaseObjects dbo;
    dbo.openDatabase(global_DBObjects.getDatabaseFile(), caller, false);
    int result = dbo.importXMLDoc(xmldoc);

    global_DBObjects.addColumnChanges(dbo);

    dbo.closeDatabase();

    emit MainWindow::getPluginManager()->pluginRefreshRequest();

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

    DatabaseObjects dbo;
    dbo.openDatabase(global_DBObjects.getDatabaseFile(), caller, false);
    QList<SqlQueryModel*>* models = dbo.getData(xmldoc);

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

    if (QFile("python.exe").exists())
    {
        QLog_Info(CONSOLELOG,QString("Setting application Python instance to isolated."));
        PyConfig_SetBytesString(&config, &config.home, "python");   // relative or absolute
        config.isolated = 1;      // No user site packages, no system Python
        config.use_environment = 0; // Prevent env vars like PATH, PYTHONPATH
    }

    PyImport_AppendInittab("projectnotes", PyInit_embeddedconsole);

    //Py_Initialize();
    Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);

    PyImport_ImportModule("projectnotes");

    stdout_write_type outwrite = [this] (std::string s)
    {
        std::string text = s;

        // ignore double return console messages
        if (text == "\n\n")
            return;

        if (text == "\n")
            return;

        QLog_Info(CONSOLELOG, QString::fromStdString(text).trimmed());
    };

    set_stdout(outwrite);

    // load PNPlugin modules — batch all path setup into a single interpreter call
    QString pathSetup;
    pathSetup.reserve(512);
    pathSetup =  "import sys\n"
                 "import inspect\n";
    pathSetup += QString("sys.path.append(\"%1\")\n").arg(m_pluginspath);
    pathSetup += QString("sys.path.append(\"%1\")\n").arg(m_threadspath);
    pathSetup += QString("sys.path.append(\"%1\")\n").arg(pythonpath + "/python313.zip");
    pathSetup += QString("sys.path.append(\"%1\")\n").arg(pythonpath + "/site-packages");
    pathSetup += QString("sys.path.append(\"%1\")\n").arg(pythonpath + "/site-packages/win32");
    pathSetup += QString("sys.path.append(\"%1\")\n").arg(pythonpath + "/site-packages/win32/lib");
    pathSetup += QString("sys.path.append(\"%1\")\n").arg(pythonpath + "/site-packages/Pythonwin");
#ifdef Q_OS_WIN
    pathSetup += "import os\n";
    pathSetup += QString("os.add_dll_directory(\"%1\")\n").arg(pythonpath + "/site-packages/PyQt6/Qt6/bin");
#endif
    PyRun_SimpleString(pathSetup.toUtf8().constData());

    QLog_Info(CONSOLELOG, QString("Embedded Python Version %1").arg(Py_GetVersion()));

    m_pythreadstate = PyEval_SaveThread();

    // Seed directory snapshot so first poll doesn't fire for already-loaded files
    auto seedDir = [this](const QString& path) {
        QDir dir(path);
        for (const QString& name : dir.entryList({"*.py"}, QDir::Files))
            m_watchedDirFiles.insert(dir.absoluteFilePath(name));
    };
    seedDir(m_pluginspath);
    seedDir(m_threadspath);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(1500);
    connect(m_pollTimer, &QTimer::timeout, this, &PluginManager::onPollTimer);
    m_pollTimer->start();

    connect(this, &PluginManager::pluginForceReload, this, &PluginManager::onForceReload);

    loadPluginFiles(m_pluginspath, false);
    loadPluginFiles(m_threadspath, true);
}


void PluginManager::loadPluginFiles(const QString& path, bool isthread)
{
    // Build a set of already-loaded paths for O(1) duplicate detection
    QSet<QString> loadedPaths;
    loadedPaths.reserve(m_pluginlist.size());
    for (const Plugin* p : m_pluginlist)
        loadedPaths.insert(p->modulepath());

    QDirIterator fileit(path, {"*.py"}, QDir::Files);

    // iterate through PNPlugins folder and load py files
    while (fileit.hasNext())
    {
        fileit.next();

        const QString filePath = fileit.fileInfo().absoluteFilePath();

        if (filePath.contains("projectnotes.py", Qt::CaseInsensitive))
        {
            QLog_Info(CONSOLELOG, QString("Ignoring module file '%1'.").arg(filePath));
            continue;
        }

        if (!loadedPaths.contains(filePath))
        {
            Plugin* module = new Plugin(this, isthread);
            m_pluginlist.append(module);

            connect(module, &Plugin::moduleLoaded, this, &PluginManager::onLoadComplete);
            connect(module, &Plugin::moduleUnloaded, this, &PluginManager::onUnloadComplete);

            m_watchedDirFiles.insert(filePath);  // track so directory poll doesn't re-trigger on error-reloads

            module->loadPlugin(filePath);
        }
    }
}

// close Plugin engine
PluginManager::~PluginManager()
{
    disconnect(this, &PluginManager::pluginForceReload, this, &PluginManager::onForceReload);

    for ( Plugin* p : m_pluginlist)
    {
        disconnect(p, &Plugin::moduleLoaded, this, &PluginManager::onLoadComplete);
        disconnect(p, &Plugin::moduleUnloaded, this, &PluginManager::onUnloadComplete);

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

void PluginManager::onUnloadComplete(const QString &modulepath)
{
    QLog_Info(CONSOLELOG, QString("Module '%1' unloaded.").arg(modulepath));

    QFileInfo file(modulepath);

    if (!file.exists())
    {
        // file is gone, close out the thread
        for (auto it = m_pluginlist.begin(); it != m_pluginlist.end();)
        {
            if ((*it)->modulepath().compare(modulepath) == 0)
            {
                m_watchedFiles.remove(modulepath);

                delete *it;
                it = m_pluginlist.erase(it);
                QLog_Info(CONSOLELOG, QString("Module '%1' no longer exists. Thread stopped.").arg(modulepath));
            }
            else
            {
                ++it;
            }
        }
    }

    emit pluginUnLoaded(modulepath);
}

void PluginManager::onFileChanged(const QString &filepath)
{
    if (filepath.contains("projectnotes.py", Qt::CaseInsensitive))
    {
        QLog_Info(CONSOLELOG, QString("Ignoring module file change '%1'.").arg(filepath));
        return;
    }

    QLog_Info(CONSOLELOG, QString("Module file '%1' changed.").arg(filepath));

    QFileInfo file(filepath);

    for (Plugin* p : m_pluginlist)
    {
        bool related_import = false;

        const QStringList& imp = p->pythonplugin().imports();
        for (const QString& importPath : imp)
        {
            if (importPath.compare(filepath, Qt::CaseInsensitive) == 0)
            {
                related_import = true;
                break;
            }
        }

        if (related_import || p->modulepath().compare(filepath, Qt::CaseInsensitive) == 0)
        {
            p->reloadPlugin();
        }
    }
}

void PluginManager::onForceReload(const QString &module)
{
    if (module.contains("projectnotes.py", Qt::CaseInsensitive))
    {
        QLog_Info(CONSOLELOG, QString("Ignoring forced reload file change '%1'.").arg(module));
        return;
    }

    QString basemodule = QFileInfo(module).baseName();

    for (auto it = m_pluginlist.begin(); it != m_pluginlist.end(); ++it)
    {
        QString checkmodule = QFileInfo((*it)->modulepath()).baseName();

        if (checkmodule.compare(basemodule, Qt::CaseInsensitive) == 0)
        {
            (*it)->reloadPlugin();
            return;
        }
    }

    QLog_Info(CONSOLELOG, QString("Module '%1' was not found. Module reload failed.").arg(module));
}

void PluginManager::onFolderChanged(const QString &folderPath)
{
    loadPluginFiles(m_pluginspath, false);
    loadPluginFiles(m_threadspath, true);
}

void PluginManager::onLoadComplete(const QString& modulepath)
{
    QLog_Info(CONSOLELOG, QString("Module file '%1' load complete.").arg(modulepath));

    QString basemodule = QFileInfo(modulepath).baseName();

    // add all of the imported modules to the file tracker
    for (const Plugin* p : m_pluginlist)
    {
        if (QFileInfo(p->modulepath()).baseName().compare(basemodule, Qt::CaseInsensitive) == 0)
        {
            m_watchedFiles.insert(modulepath, QFileInfo(modulepath).lastModified());
            for (const QString& importPath : p->pythonplugin().imports()) {
                if (!m_watchedFiles.contains(importPath))
                    m_watchedFiles.insert(importPath, QFileInfo(importPath).lastModified());
            }
        }
    }

    emit pluginLoaded(modulepath);
}

void PluginManager::forceReload(const QString& module)
{
    emit pluginForceReload(module);
}

int PluginManager::loadedCount()
{
    int loaded_count = 0;

    for (auto p : plugins())
        if (p->loaded())
            loaded_count++;

    return(loaded_count);
}

void PluginManager::onPollTimer()
{
    // Part 1: Detect new .py files in plugins/ and threads/ directories
    bool folderChanged = false;
    auto checkDir = [this, &folderChanged](const QString& path) {
        QDir dir(path);
        for (const QString& name : dir.entryList({"*.py"}, QDir::Files)) {
            const QString fullPath = dir.absoluteFilePath(name);
            if (!m_watchedDirFiles.contains(fullPath)) {
                m_watchedDirFiles.insert(fullPath);
                folderChanged = true;
            }
        }
    };
    checkDir(m_pluginspath);
    checkDir(m_threadspath);
    if (folderChanged)
        onFolderChanged(QString());  // onFolderChanged ignores its argument

    // Part 2: Detect changes to watched plugin files and their imports
    for (auto it = m_watchedFiles.begin(); it != m_watchedFiles.end(); ++it) {
        QFileInfo fi(it.key());
        if (!fi.exists())
            continue;  // file mid-deletion; wait for recreation
        const QDateTime newMod = fi.lastModified();
        if (newMod != it.value()) {
            it.value() = newMod;
            onFileChanged(it.key());
        }
    }
}
