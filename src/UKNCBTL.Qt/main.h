#ifndef MAIN_H
#define MAIN_H

//////////////////////////////////////////////////////////////////////

class QApplication;
class QMainWindow;
class QSettings;


QApplication* Global_getApplication();
QMainWindow* Global_getMainWindow();
QSettings* Global_getSettings();


//////////////////////////////////////////////////////////////////////
// Settings

void Settings_SetFloppyFilePath(int slot, LPCTSTR sFilePath);
QString Settings_GetFloppyFilePath(int slot);
void Settings_SetCartridgeFilePath(int slot, LPCTSTR sFilePath);
QString Settings_GetCartridgeFilePath(int slot);
void Settings_SetHardFilePath(int slot, LPCTSTR sFilePath);
QString Settings_GetHardFilePath(int slot);


//////////////////////////////////////////////////////////////////////
#endif // MAIN_H