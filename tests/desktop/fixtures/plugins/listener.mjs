import { data, ui } from "projectnotes";
data.onChanged("contacts", change => ui.message(JSON.stringify(change)));
