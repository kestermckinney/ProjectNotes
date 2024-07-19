# Global Settings Plugin

# Sharing Settings Across All Plugins
Each plugin can have their own [Plugin Settings](<../StandardPlugins/PluginSettings.md>). The settings are determined by the contents of the Python file. The plugins setting structure is very flexible, however many plugins require the same settings. The Global Settings plugin is a very basic plugin with no events. It only stores settings values. The standard plugin only stores the **ProjectsFolder** location, and **DefaultMeetingLocation**.