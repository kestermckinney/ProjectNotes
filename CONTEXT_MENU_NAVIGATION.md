# Context Menu Navigation Enhancement

## Overview
Added "Go To Person" and "Go To Client" context menu actions to enable quick navigation between related records across the application.

## Features Implemented

### 1. People Page → Client Navigation
- **Location**: `ProjectNotesDesktop/qml/pages/PeoplePage.qml`
- **Action**: Right-click on a person → "Go To Client"
- **Behavior**: Jumps to the Clients page and opens the client record associated with that person
- **Prerequisites**: Person must have a client_id set

### 2. Project Team Member → Person Navigation
- **Location**: `ProjectNotesDesktop/qml/pages/ProjectDetailPage.qml` (Team tab)
- **Action**: Right-click on a team member → "Go To Person"
- **Behavior**: Jumps to the People page and opens the person's full record
- **Prerequisites**: Always available for team members (people_id is always set)

## Technical Implementation

### Modified Files

#### 1. `RecordContextMenu.qml`
- Added navigation signals: `goToPersonRequested(string personId)`, `goToClientRequested(string clientId)`
- Added properties: `personId`, `clientId`, `canGoToPerson`, `canGoToClient`
- Added two new menu rows with Material Design icons:
  - "Go To Person" (icon: "person")
  - "Go To Client" (icon: "apartment")
- Dividers are shown conditionally based on visibility of navigation actions

#### 2. `PeoplePage.qml`
- Added signal: `goToClientRequested(string clientId)`
- Connected RecordContextMenu's `goToClientRequested` signal to page signal
- Set `clientId` property when opening context menu based on `card.model.client_id`

#### 3. `RecordRowMenu.qml`
- Extended to support navigation actions (inherits from RecordContextMenu)
- Added signals: `goToPersonRequested(string personId)`, `goToClientRequested(string clientId)`
- Updated `openFor()` method to accept and set optional `personId` and `clientId` parameters
- Connected base signals to row menu signals

#### 4. `ProjectDetailPage.qml`
- Added signals: `goToPersonRequested(string personId)`, `goToClientRequested(string clientId)`
- Updated team member context menu call to pass `people_id` as `personId` parameter:
  ```qml
  rowMenu.openFor(..., (teamCard.model.people_id || "").toString())
  ```
- Connected RecordRowMenu's navigation signals to page signals

#### 5. `Main.qml`
- Updated `peopleComponent` to handle `onGoToClientRequested`:
  - Gets client row ID
  - Switches to Clients section
  - Opens the client detail page
- Updated `projectDetailComponent` to handle both navigation signals:
  - `onGoToPersonRequested`: Gets person row, switches to People section, opens person detail
  - `onGoToClientRequested`: Gets client row, switches to Clients section, opens client detail

## User Workflow Examples

### Example 1: From People List to Client
1. User is viewing the People list
2. Right-clicks on "John Doe" (who works for "Acme Corp")
3. Selects "Go To Client" from context menu
4. Application switches to Clients page
5. "Acme Corp" client detail page opens automatically

### Example 2: From Project Team to Person
1. User is viewing a Project's Team tab
2. Right-clicks on "Jane Smith" (team member)
3. Selects "Go To Person" from context menu
4. Application switches to People page
5. "Jane Smith" person detail page opens automatically

## Icon Choices
- **"Go To Person"**: Uses "person" Material Design icon
- **"Go To Client"**: Uses "apartment" Material Design icon (consistent with existing client icons)

## Menu Organization
- Navigation actions appear in their own section, separated by dividers
- Dividers only show when navigation actions are visible
- Navigation actions appear before Export/Filter/Refresh section
- Maintains consistent menu hierarchy and visual organization

## Future Extension Points
- The infrastructure supports adding "Go To Project" or other navigation actions
- Any page displaying people or clients can pass `personId`/`clientId` to context menus
- Other detail pages (ItemDetailPage, etc.) can leverage the same navigation pattern
