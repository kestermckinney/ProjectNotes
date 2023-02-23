#include "pnplugin.h"

#include <QObject>
#include <QString>
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>

bool PNPlugin::loadModule(const QFileInfo& t_filename)
{
    qDebug() << "Loading plugin file: " << t_filename;
    m_PluginLocation = t_filename.absoluteFilePath();

    m_PNPluginModule = PyImport_ImportModule(t_filename.baseName().toUtf8().constData());
    if (!m_PNPluginModule)
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Module Failed", QString("Could not load module %1." ).arg(t_filename.absoluteFilePath()));
        return false;
    }

    m_PNPluginName = getPythonVariable("pluginname");
    if (m_PNPluginName.isNull())
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Module Failed", QString("PNPlugin name is required for module %1." ).arg(t_filename.absoluteFilePath()));
        return false;
    }

    m_Description = getPythonVariable("plugindescription");
    if (m_Description.isNull())
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Module Failed", QString("PNPlugin description is required for module %1." ).arg(t_filename.absoluteFilePath()));
        return false;
    }

    m_TableName = getPythonVariable("plugintable");
    m_ChildTablesFilter = getPythonVariable("childtablesfilter");

    m_Parameters = getPythonStringList("parameters");

    //TODO: setup events to be callable
    m_Startup = PyObject_GetAttrString(m_PNPluginModule,"event_startup");
    m_Shutdown = PyObject_GetAttrString(m_PNPluginModule,"event_shutdown");
    m_EveryMinute = PyObject_GetAttrString(m_PNPluginModule,"event_everyminute");
    m_Every5Minutes = PyObject_GetAttrString(m_PNPluginModule,"event_every5minutes");
    m_Every10Minutes = PyObject_GetAttrString(m_PNPluginModule,"event_every10minutes");
    m_Every30Minutes = PyObject_GetAttrString(m_PNPluginModule,"event_every30minutes");
    m_PNPluginMenu = PyObject_GetAttrString(m_PNPluginModule,"event_menuclick");

    m_DataRightClickEvent = PyObject_GetAttrString(m_PNPluginModule,"event_data_rightclick");

    return true;
}

QStringList PNPlugin::getEventNames()
{
    QStringList val;

    if (m_Startup) val.append("event_startup");
    if (m_Shutdown) val.append("event_shutdown");
    if (m_EveryMinute) val.append("event_everyminute");
    if (m_Every5Minutes) val.append("event_every5minutes");
    if (m_Every10Minutes) val.append("event_every10minutes");
    if (m_Every30Minutes) val.append("event_every30minutes");
    if (m_PNPluginMenu) val.append("event_menuclick");

    QString rc("event_data_rightclick on " + m_TableName);
    if (!m_ChildTablesFilter.isEmpty()) rc += " filtered by " + m_ChildTablesFilter;

    if (m_DataRightClickEvent) val.append(rc);

    return val;
}

PNPlugin::~PNPlugin()
{
    Py_XDECREF(m_Startup);
    Py_XDECREF(m_Shutdown);
    Py_XDECREF(m_EveryMinute);
    Py_XDECREF(m_Every5Minutes);
    Py_XDECREF(m_Every10Minutes);
    Py_XDECREF(m_Every30Minutes);
    Py_XDECREF(m_PNPluginMenu);

    Py_XDECREF(m_DataRightClickEvent);
    Py_XDECREF(m_PNPluginModule);
}

bool PNPlugin::hasDataRightClickEvent(const QString& t_tablename)
{
    bool val = ( m_DataRightClickEvent != nullptr  &&
                QString::compare(m_TableName, t_tablename, Qt::CaseInsensitive) == 0);
    return val;
};

void PNPlugin::callStartupEvent()
{
    callPythonMethod(m_Startup);
}

void PNPlugin::callShutdownEvent()
{
    callPythonMethod(m_Shutdown);
}

void PNPlugin::callEveryMinuteEvent()
{
    callPythonMethod(m_EveryMinute);
}

void PNPlugin::callEvery5MinutesEvent()
{
    callPythonMethod(m_Every5Minutes);
}

void PNPlugin::callEvery10MinutesEvent()
{
    callPythonMethod(m_Every10Minutes);
}

void PNPlugin::callEvery15MinutesEvent()
{
    callPythonMethod(m_Every15Minutes);
}

void PNPlugin::callEvery30MinutesEvent()
{
    callPythonMethod(m_Every30Minutes);
}

void PNPlugin::callPNPluginMenuEvent()
{
    callPythonMethod(m_PNPluginMenu);
}

QString PNPlugin::callDataRightClickEvent(const QString& t_xmlstring)
{
    return callPythonMethod(m_DataRightClickEvent, t_xmlstring);
}

QString PNPlugin::callPythonMethod(PyObject* t_method)
{
    char* result;
    QString val;

    //TODO: CALL a set parameters for all active parameters
    for ( QString p : m_Parameters)
    {
        //TODO: get the parameter value stored in QConfig
        QString parmval = "big param";

        setPythonVariable(p, parmval);
    }

    PyObject* func = PyObject_CallObject(t_method, nullptr);
    if (!func)
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", "Could not call method.");
        return QString();
    }

    if (!PyArg_Parse(func, "s", &result))                /* convert to C */
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", "Could not get result for method.");
        Py_XDECREF(func);
        return QString();
    }

    val = QString(result);

    Py_XDECREF(func);

    return val;
}

QString PNPlugin::callPythonMethod(PyObject* t_method, const QString& t_xmlstring)
{
    char* result;
    QString val;

    //TODO: CALL a set parameters for all active parameters
    for ( QString p : m_Parameters)
    {
        //TODO: get the parameter value stored in QConfig
        QString parmval = "small parm value";

        setPythonVariable(p, parmval);
    }

    qDebug() << "Debug UTF: " << t_xmlstring;

    PyObject* pargs = Py_BuildValue("(s)", t_xmlstring.toUtf8().data());
    if (!pargs)
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", "Could not build argument list for Python method.");
        return QString();
    }

    PyObject* func = PyObject_CallObject(t_method, pargs);
    if (!func)
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", "Could not call Python method.");
        Py_XDECREF(pargs);
        return QString();
    }

    Py_XDECREF(pargs);

    if (!PyArg_Parse(func, "s", &result))                /* convert to C */
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", "Could not get result from Python method.");
        Py_XDECREF(pargs);
        Py_XDECREF(func);
        return QString();
    }

    val = QString(result);

    Py_XDECREF(func);

    return val;
}

int PNPlugin::setPythonVariable(const QString& t_variablename, const QString& t_value)
{
    qDebug() << "Setting Value: "  << t_variablename << " as " << t_value;

    PyObject* objectval = PyUnicode_FromString(t_value.toStdString().c_str());

    int r = PyObject_SetAttrString(m_PNPluginModule, t_variablename.toStdString().c_str(), objectval);
    if (r == -1)
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", QString("Could not set attribute %1.").arg(t_variablename) );
        return -1;
    }

    Py_XDECREF(objectval);
    return r;
}

QString PNPlugin::getPythonVariable(const QString& t_variablename)
{
    PyObject* attr = PyObject_GetAttrString(m_PNPluginModule, t_variablename.toStdString().c_str());
    if (!attr)
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", QString("Could not get attribute %1.").arg(t_variablename) );
        return QString();
    }

    const char* str = PyUnicode_AsUTF8(attr);
    if (!str)
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", QString("Could not get string for %1.").arg(t_variablename) );
        return QString();
    }

    Py_XDECREF(attr);

    return QString(str);
}

QStringList PNPlugin::getPythonStringList(const QString& t_variablename)
{
    QStringList val;

    PyObject* attr = PyObject_GetAttrString(m_PNPluginModule, t_variablename.toStdString().c_str());
    if (!attr)
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", QString("Could not get attribute %1.").arg(t_variablename) );
        return QStringList();
    }


    if (!PyList_Check(attr))
    {
        PyErr_PrintEx(0);
        QMessageBox::critical( nullptr, "Python Call Failed", QString("Could not get attribute %1.").arg(t_variablename) );
        Py_XDECREF(attr);
        return QStringList();
    }

    Py_ssize_t sz = PyList_Size(attr);

    for (long i = 0; i < sz; i++)
    {
        PyObject* item = PyList_GetItem(attr, i);
        if (!item)
        {
            PyErr_PrintEx(0);
            QMessageBox::critical( nullptr, "Python Call Failed", QString("Could not get item %1 for %2.").arg(i).arg(t_variablename) );
            return QStringList();
        }

        const char* str = PyUnicode_AsUTF8(item);

        val.append(QString(str));

        Py_XDECREF(item);
    }

    Py_XDECREF(attr);

    return val;
}
