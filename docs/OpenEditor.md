# Open Editor

## Editing Script Plugins
If you intend on customizing Project Notes by changing the current plugins or creating your own, you will want to use an editor that supports Python syntax highlighting and auto completion features. The Open Editor plugin allows you to specify the editor executable an parameters to be called from the Plugins menu.

**Using Atom As Your Editor**
One of the more popular editors is Atom ([https://atom.io/](<https://atom.io/>)). Atom is very easy to customize. It has several plugins that support editing and debugging lua. Some plugins also support syntax highlighting and auto-completion. When specifying the editor command below use "atom ./". This command will open the editor and display the plugin folder that contains all of the Python scripts.

**To setup an editor:**
1. From the **Plugins** menu, click **Plugin Settings**.
2. Choose **Open Editor** in the plugins list.
3. Type the editor command line in the **EditorFullPath** property.
4. Click **Apply**.
