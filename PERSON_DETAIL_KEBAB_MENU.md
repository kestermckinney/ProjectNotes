# PersonDetailPage Kebab Menu Implementation

## Overview

Added a three-dot (kebab) menu button to the PersonDetailPage that displays:
1. **Export XML** - Export person record
2. **Go To Client** - Navigate to associated client (if assigned)
3. **Refresh** - Reload person data from database
4. **Plugins** - Any plugin actions configured for people records

## Changes Made

### 1. PersonDetailPage.qml

**Added Signal (Line 21):**
```qml
signal goToClientRequested(string clientId)
```

**Added Function (Lines 53-57):**
```qml
function _openSelfMenu(sx, sy) {
    selfMenu.recordLabel = (nameField.text || "").toString()
    selfMenu.clientId = page._clientId
    selfMenu.openAt(sx, sy)
}
```

**Modified UI Header (Lines 69-82):**
- Wrapped name field in a RowLayout
- Added KebabButton next to name field
- KebabButton triggers `_openSelfMenu` on click
- KebabButton positioned at bottom-right of name field row

**Added RecordContextMenu (Lines 99-116):**
```qml
RecordContextMenu {
    id: selfMenu
    recordType: qsTr("Person")
    model: DesktopAppController.peopleModel
    recordId: page.personId
    canOpen: false
    canNew: false
    canDelete: false
    canDuplicate: false
    canMoveTo: false
    canExport: true
    canFilter: false
    canRefresh: true
    onRefreshRequested: page._reload()
    onGoToClientRequested: page.goToClientRequested(clientId)
}
```

**Configuration:**
- `canExport: true` - Export XML option shown
- `canRefresh: true` - Refresh option shown
- `canFilter: false` - No filter option (detail page context)
- `canOpen/New/Delete/Duplicate/MoveTo: false` - Not applicable for person detail
- Model set to `peopleModel` to enable plugin menus for people
- clientId dynamically set when menu opens to show "Go To Client" action

### 2. Main.qml

**Updated personDetailComponent (Lines 643-653):**
```qml
Component {
    id: personDetailComponent
    PersonDetailPage {
        onGoToClientRequested: (clientId) => {
            var row = DesktopAppController.clientRowForId(clientId)
            if (row >= 0) {
                root.selectSection("clients")
                root.openClient(row, clientId)
            }
        }
    }
}
```

**Navigation Logic:**
- Handles "Go To Client" request from PersonDetailPage
- Looks up client row by ID
- Switches to Clients section
- Opens client detail page if found

## User Workflow

1. User is viewing a person's details
2. Clicks the three-dot (kebab) button in the top-right of the Name field
3. Context menu opens showing:
   - "Export XML..." - Export this person's record
   - "Go To Client" (if person has client assigned) - Jump to client page
   - "Refresh" - Reload data
   - "[Plugins]" (if any plugins registered for people)
4. User selects an action

## Menu Behavior

**Export:** Triggered through parent Main.qml handler (via route to exportRecord)  
**Go To Client:** Only visible when `_clientId !== ""` (client is assigned)  
**Refresh:** Calls `page._reload()` to refresh person data from database  
**Plugins:** Automatically discovered and shown by RecordContextMenu based on registered plugin menus for "people" table  

## Visual Design

- **Button Style:** Three-dot Material Design icon (KebabButton)
- **Position:** Top-right of Name field, vertically aligned with field bottom
- **Spacing:** 6px bottom margin to align with form field
- **Menu Appearance:** Matches RecordContextMenu styling (rounded, with dividers, Material Design icons)

## Feature Integration

✅ Consistent with ProjectDetailPage pattern (uses same RecordContextMenu)  
✅ Supports plugins for people records  
✅ Integrates with new "Go To Client" navigation feature  
✅ Follows existing UI patterns and styling  
✅ Graceful menu organization (dividers between action groups)  

## Code Quality

✅ No new dependencies added  
✅ Uses existing DesktopAppController methods  
✅ Follows QML property binding patterns  
✅ Signal forwarding is clean and simple  
✅ Menu configuration is explicit and maintainable  

## Testing Checklist

- [ ] Build in Qt Creator
- [ ] Navigate to a person's detail page
- [ ] Click the kebab button (three dots)
- [ ] Verify menu shows "Export XML...", "Go To Client" (if client assigned), and "Refresh"
- [ ] Click "Go To Client" and verify navigation to client page
- [ ] Click "Refresh" and verify person data reloads
- [ ] Click "Export XML..." and verify export dialog opens
- [ ] If plugins exist for people, verify they appear in menu

## Files Modified

| File | Changes | Status |
|------|---------|--------|
| PersonDetailPage.qml | Added signal, function, UI button, menu | Ready |
| Main.qml | Added signal handler for navigation | Ready |

**Ready for testing!**
