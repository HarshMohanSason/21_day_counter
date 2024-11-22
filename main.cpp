#include <windows.h>
#include "resource.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <vector>
#include "guest.h"
#include <commctrl.h>
extern "C" {
    #include "sqlite3.h"
}

using namespace std;

//---------------Some global variables--------------

//These will be used entirely for fetching values from dialog boxes or updating the values in the db. 

char g_GuestName[100];
char g_FolioNumber[100];
char g_CheckInDate[100];
char g_CheckOutDate[100];
int total_days_stayed;

//Program name
const char g_szClassName[] = "21 Day Counter";

// global current instance
HINSTANCE hInst;

// global instance for the listView; 
static HWND hWndListView;

//call back function for adding a new guest 
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);

void UpdateListAfterAddingNewGuest(HWND hWndListView, const Guest& newGuest){
      
    LV_ITEM lvI;
    lvI.mask = LVIF_TEXT |LVIF_PARAM | LVIF_STATE;
    lvI.state = 0; //
    lvI.stateMask = 0; //

    // Insert the new guest data into the ListView
    int rowIndex = ListView_GetItemCount(hWndListView);  // Get the current number of items in the ListView

    lvI.iItem = rowIndex;  // The next available row index

    // Set the text for the first column (Guest Name)
    lvI.iSubItem = 0;
    lvI.pszText = (char*)newGuest.name.c_str();
    ListView_InsertItem(hWndListView, &lvI);  // Insert the item into the ListView

    // Set text for other columns
    ListView_SetItemText(hWndListView, rowIndex, 1, (char*)newGuest.folioNumber.c_str());  // Folio Number
    ListView_SetItemText(hWndListView, rowIndex, 2, (char*)newGuest.checkInDate.c_str());  // Check-in Date
    ListView_SetItemText(hWndListView, rowIndex, 3, (char*)newGuest.checkOutDate.c_str());  // Check-out Date

    // For the number of days stayed, convert it to a string and set it
    string str = to_string(newGuest.totalDaysStayed);
    ListView_SetItemText(hWndListView, rowIndex, 4,(char*) str.c_str());
ListView_RedrawItems(hWndListView, rowIndex, rowIndex);  // Redraw just the new row

// Optionally, you can also force the ListView to refresh if necessary
UpdateWindow(hWndListView);
}

void AddDataInDb(Guest& guest) {
    sqlite3 * db;
    int rc;

    // Open the database
    rc = sqlite3_open("guest_information.db", & db);
    if (rc) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error opening database", MB_OK | MB_ICONERROR);
        return;
    }

    // Create table (if not exists)
    const char * sql_create_table = "CREATE TABLE IF NOT EXISTS guests ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "guest_name TEXT,"
    "folio_number TEXT,"
    "check_in_date TEXT,"
    "check_out_date TEXT,"
    "days_stayed INTEGER)";
    rc = sqlite3_exec(db, sql_create_table, nullptr, nullptr, nullptr);
    if (rc) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error creating table", MB_OK | MB_ICONERROR);
        sqlite3_close(db);
        return;
    }

    // Prepare insert statement
    const char * sql_insert = "INSERT INTO guests (guest_name, folio_number, check_in_date, check_out_date, days_stayed) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt * stmt;
    rc = sqlite3_prepare_v2(db, sql_insert, -1, & stmt, nullptr);

    if (rc == SQLITE_OK) {
        // Bind data to statement
        sqlite3_bind_text(stmt, 1, guest.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, guest.folioNumber.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, guest.checkInDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, guest.checkOutDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, guest.totalDaysStayed);

        // Execute the insert statement
        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE) {
            MessageBox(NULL, sqlite3_errmsg(db), "Error inserting data", MB_OK | MB_ICONERROR);
        } else {
            // Success message (optional)
            // MessageBox(NULL, "Data inserted successfully!", "Success", MB_OK);
        }

        sqlite3_finalize(stmt);
    } else {
        MessageBox(NULL, sqlite3_errmsg(db), "Error preparing statement", MB_OK | MB_ICONERROR);
    }

    sqlite3_close(db);
}

HWND CreateListView(HWND hWndParent) {

    HWND hWndList; // Handle to the list view window
    RECT rcl; // Rectangle for setting the size of the window
    // Ensure that the common control DLL is loaded.
    int index;
    char szText[MAX_PATH]; // Place to store some text
    LV_COLUMN lvC; // List View Column structure
    LV_ITEM lvI;        // List view item structure
    InitCommonControls(); //Initialize the common controls

    // Get the size and position of the parent window
    GetClientRect(hWndParent, & rcl);
    hWndList = CreateWindowEx(0L,
        WC_LISTVIEW, // list view class
        "", // no default text
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT |
        LVS_EDITLABELS | WS_EX_CLIENTEDGE,
        0, 0,
        rcl.right - rcl.left, rcl.bottom - rcl.top,
        hWndParent,
        (HMENU) 81,
        hInst,
        NULL);


    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT; // left align the column
    lvC.cx = 140; // width of the column, in pixels
    lvC.pszText = szText;

    //Set the column header values 
    for (index = 0; index <= 5; index++) {
        lvC.iSubItem = index;
        LoadString(hInst,
            IDS_GUEST_NAME + index,
            szText,
            sizeof(szText)); //Load the values from the resource file 
        if (ListView_InsertColumn(hWndList, index, & lvC) == -1)
            return NULL;
    }

    lvI.mask = LVIF_TEXT |LVIF_PARAM | LVIF_STATE;
    lvI.state = 0; //
    lvI.stateMask = 0; //

    sqlite3 * db;
    int rc;

    rc = sqlite3_open("guest_information.db", & db); //open the db

    if (rc) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error opening database", MB_OK | MB_ICONERROR);
     }

    const char * sql = "SELECT * FROM guests"; //select all the values from the table guest
    sqlite3_stmt * stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, & stmt, nullptr);

 
    int rowIndex = 0;  // Row index for ListView

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

    lvI.iItem = rowIndex;  // Row index in ListView

    lvI.iSubItem = 0;  // First column (name of the guest)
    lvI.pszText = (char*)sqlite3_column_text(stmt, 1); //Guest name in the column is the 1st value in column in the db
    ListView_InsertItem(hWndList, &lvI);  // Insert row into ListView
    
    // For the remaining columns (2 to 5), set the values
    for (int i = 2; i <=5; i++) {
        lvI.iSubItem = i-1;
        
        if(i == 5) //the last value is no of days which is an integer
	{
           int no_of_days_stayed = sqlite3_column_int(stmt, i);  

           // Convert the integer to a string
           string str = to_string(no_of_days_stayed);

           //set the pszText value 
           lvI.pszText = (char*) str.c_str();
	}

	else{
        const char* columnText = (const char*)sqlite3_column_text(stmt, i);
        if (columnText) {
            lvI.pszText = (char*)columnText;  // Set the text value for the column
        }
	}

        // Update the corresponding ListView item
        ListView_SetItemText(hWndList, rowIndex, i-1, lvI.pszText);
    }

    rowIndex++;  // Increment row index for the next item
}

}


//Callback for the main window
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hWndListView = CreateListView(hwnd);
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case ID_ADD_NEW_GUEST: {
            //Create the dialog box to add new guest
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ADD_NEW_GUEST_DIALOG), hwnd, AboutDlgProc);

        }
        break;
        }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    switch (Message) {

    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK_ADD_NEW_GUEST: {
            //Get all the values from the edit boxes
            GetDlgItemText(hwnd, ID_GUEST_NAME, g_GuestName, 100);
            GetDlgItemText(hwnd, ID_FOLIO_NUMBER, g_FolioNumber, 100);
            GetDlgItemText(hwnd, ID_CHECK_IN_DATE, g_CheckInDate, 100);
            GetDlgItemText(hwnd, ID_CHECK_OUT_DATE, g_CheckOutDate, 100);
	    //For some reason GetDlgItemInt does not return the integer value inside the box, so i am using char array then converting it to int to store in db
	    char total_days[50]; 
            GetDlgItemText(hwnd, ID_NO_OF_DAYS_STAYED, total_days, 50);
 		
	    total_days_stayed = stoi(total_days);
            //create instance for the guest information
            Guest instance = Guest(
                string(g_GuestName),
                string(g_FolioNumber),
                string(g_CheckInDate),
                string(g_CheckOutDate),
                total_days_stayed
            );
            AddDataInDb(instance); //add the data to the db
            UpdateListAfterAddingNewGuest(hWndListView, instance);
            EndDialog(hwnd, IDOK_ADD_NEW_GUEST);
            break;
        }
        case IDCANCEL_ADD_NEW_GUEST:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;
    //Step 1: Registering the Window Class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassEx( & wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    hInst = hInstance;
    // Step 2: Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "21 Day Counter",
        WS_OVERLAPPEDWINDOW,
        0, 0, 700, 600,
        NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    // Step 3: The Message Loop
    while (GetMessage( & Msg, NULL, 0, 0) > 0) {
        TranslateMessage( & Msg);
        DispatchMessage( & Msg);
    }
    return Msg.wParam;
}