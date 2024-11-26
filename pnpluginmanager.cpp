// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pnpluginmanager.h"

#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
#include <QMessageBox>
//#include <QDebug>
#include <QStandardPaths>
#include <QThread>
#include "pndatabaseobjects.h"

#include <sstream>

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

static PyObject* update_data(PyObject* self, PyObject* args)
{
    Q_UNUSED(self);

    const char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
    {
        return PyBool_FromLong(0);
    }

    std::thread::id threadId = std::this_thread::get_id();
    std::stringstream ss;
    ss << threadId;

    qDebug() << "Current thread ID: " << ss.str();

    QDomDocument xmldoc;
    QByteArray ba(input);

    xmldoc.setContent(ba);

    PNDatabaseObjects dbo;
    dbo.openDatabase(global_DBObjects.getDatabaseFile());
    long result = dbo.importXMLDoc(xmldoc);
    dbo.closeDatabase();

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

    QDomDocument xmldoc;
    QByteArray ba(input);

    xmldoc.setContent(ba);

    qDebug() << "Embedded Thread " << QThread::currentThreadId();

    PNDatabaseObjects dbo;
    dbo.openDatabase(global_DBObjects.getDatabaseFile());
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

    delete returnxmldoc;

    PyObject* pyString = Py_BuildValue("s", xmlstring.toUtf8().constData());
    return pyString;
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
    // 0,                    /* tp_weaklist */
    // 0,                    /* tp_del */
    // 0,                    /* tp_version_tag */
    // 0,                    /* tp_finalize */
    // 0,                    /* tp_vectorcall */
    // 0,                    /* tp_watched */
};

PyModuleDef embmodule =
{
    PyModuleDef_HEAD_INIT,
    "projectnotes",
    "Project Notes module callable from embeded Python",
    -1,
    data_methods//, 0, 0, 0, 0,
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

// initialize PNPlugin engine
PNPluginManager::PNPluginManager(QWidget* t_parent)
{
    Py_SetProgramName(L"Project Notes 3");

    PyImport_AppendInittab("projectnotes", PyInit_embeddedconsole);

    Py_Initialize();

    PyImport_ImportModule("projectnotes");
    m_console_dialog = new PNConsoleDialog(t_parent);

    // load PNPlugin modules
    QString fullpath = QCoreApplication::applicationDirPath() + "/plugins/";
    QString syspath = QString("sys.path.append(\"%1\")").arg(fullpath);
    QString pythonzip = QString("sys.path.append(\"%1\")").arg(QCoreApplication::applicationDirPath() + "/python311.zip");
    QString sitepackages = QString("sys.path.append(\"%1\")").arg(QCoreApplication::applicationDirPath() + "/site-packages");
    QString win32path = QString("sys.path.append(\"%1\")").arg(QCoreApplication::applicationDirPath() + "/site-packages/win32");
    QString win32lib = QString("sys.path.append(\"%1\")").arg(QCoreApplication::applicationDirPath() + "/site-packages/win32/lib");
    QString pythonwin = QString("sys.path.append(\"%1\")").arg(QCoreApplication::applicationDirPath() + "/site-packages/Pythonwin");

    PyRun_SimpleString("import sys");
    PyRun_SimpleString(syspath.toUtf8().constData());
    PyRun_SimpleString(pythonzip.toUtf8().constData());
    PyRun_SimpleString(sitepackages.toUtf8().constData());
    PyRun_SimpleString(win32path.toUtf8().constData());
    PyRun_SimpleString(win32lib.toUtf8().constData());
    PyRun_SimpleString(pythonwin.toUtf8().constData());

    QDirIterator fileit( fullpath, {"*.py"}, QDir::Files);

    //qDebug() << "Plugin Path: " << fullpath;

    // iterate through PNPlugins folder and load py files
    while (fileit.hasNext())
    {
        fileit.next();

        PNPlugin* module = new PNPlugin();

        if (module->loadModule(fileit.fileInfo()))
            m_PNPlugins.append(module);
    }

    sortPlugins();
}

// close PNPlugin engine
PNPluginManager::~PNPluginManager()
{
    for ( PNPlugin* p : m_PNPlugins)
        delete p;

    m_PNPlugins.clear();

    delete m_console_dialog;

    Py_FinalizeEx();
}

QList<PNPlugin*> PNPluginManager::findStartupEvents()
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasStartupEvent())
            list.append(p);
    }

    return list;
}

QList<PNPlugin*> PNPluginManager::findShutdownEvents()
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasShutdownEvent())
            list.append(p);
    }

    return list;
}

QList<PNPlugin*> PNPluginManager::findEveryMinuteEvents()
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasEveryMinuteEvent())
            list.append(p);
    }

    return list;
}

QList<PNPlugin*> PNPluginManager::findEvery5MinutesEvents()
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasEvery5MinutesEvent())
            list.append(p);
    }

    return list;
}

QList<PNPlugin*> PNPluginManager::findEvery10MinutesEvents()
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasEvery10MinutesEvent())
            list.append(p);
    }

    return list;
}

QList<PNPlugin*> PNPluginManager::findEvery15MinutesEvents()
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasEvery15MinutesEvent())
            list.append(p);
    }

    return list;
}

QList<PNPlugin*> PNPluginManager::findEvery30MinutesEvents()
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasEvery30MinutesEvent())
            list.append(p);
    }

    return list;
}

QList<PNPlugin*> PNPluginManager::findPluginMenuEvents()
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasPNPluginMenuEvent())
            list.append(p);
    }

    return list;
}

QList<PNPlugin*> PNPluginManager::findDataRightClickEvents(const QString& t_tablename)
{
    QList<PNPlugin*> list;

    for ( PNPlugin* p : m_PNPlugins)
    {
        if (p->hasDataRightClickEvent(t_tablename))
            list.append(p);
    }

    return list;
}

void PNPluginManager::sortPlugins()
{
    int n;
    int i;

    for ( n = 0; n < m_PNPlugins.count(); n++ )
    {
        for ( i = n + 1; i < m_PNPlugins.count(); i++ )
        {
            QString valorN = m_PNPlugins.at(n)->getPNPluginName();
            QString valorI = m_PNPlugins.at(i)->getPNPluginName();

            if (valorN.toUpper() > valorI.toUpper())
            {
                m_PNPlugins.move(i, n);
                n=0;
            }
        }
    }
}
