# Spell Checking

Text fields such as note titles and the note body are checked for spelling as you type. Misspelled words are underlined with a red squiggle directly in the field, similar to a word processor.

## Correcting a Word

**To correct a flagged word:**

1. Right-click the underlined word.
2. Choose one of the suggested spellings from the menu to replace it, or choose **Add to Dictionary** to keep the word as-is and stop it being flagged in the future.

If no suggestions are available, the menu shows **(no suggestions)** instead. Right-clicking a word that isn't misspelled opens the same menu without the spelling suggestions.

## Editing Commands

The same right-click menu also has standard text-editing commands, below the spelling suggestions: **Undo**, **Redo**, then **Cut**, **Copy**, **Paste**, **Paste Unformatted**, **Delete**, and **Select All**. Cut/Copy/Delete need a selection first, and Paste/Paste Unformatted need something on the clipboard — each is grayed out otherwise.

**Paste Unformatted** inserts the clipboard's plain text at the cursor, stripping out any fonts, colors, or other formatting it carried — useful when pasting from a web page or another document into a note without carrying its styling along.

## Checking an Entire Field

For a full pass over everything you've written, use **Check Spelling…**, either from the right-click menu or from the spell-check button on the note formatting toolbar.

**To run a full check:**

1. Open **Check Spelling…**. The dialog selects the first misspelled word it finds and highlights it in the field.
2. For each flagged word, choose one of:
   - **Change** — replace just this occurrence with the text in the **Change to** field (edit it, or pick a suggestion from the list, before clicking).
   - **Change All** — replace every occurrence of that word in the field.
   - **Ignore Once** — leave this occurrence as-is and move to the next word.
   - **Ignore All** — stop flagging that word for the rest of the current session.
   - **Add to Dictionary** — accept the word permanently.
3. When no misspelled words remain, the dialog reports **Spell check complete.** Click **Done** to close it.

## The Personal Dictionary

Words added via **Add to Dictionary** are saved to a personal word list and remembered across sessions, so a word you've accepted once will not be flagged again in any note or field. Words dismissed with **Ignore Once** or **Ignore All** are not saved this way — they will be flagged again the next time the application is started.
