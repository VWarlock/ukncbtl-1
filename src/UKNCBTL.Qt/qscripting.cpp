#include "stdafx.h"
#include <QApplication>
#include <QCloseEvent>
#include "main.h"
#include "mainwindow.h"
#include "Emulator.h"
#include "emubase/Processor.h"
#include "qscripting.h"


//////////////////////////////////////////////////////////////////////
// QEmulator

QEmulator::QEmulator(QScriptWindow * window) :
    m_window(window),
    m_cpu(g_pBoard->GetCPU()),
    m_ppu(g_pBoard->GetPPU())
{
}

void QEmulator::reset()
{
    Emulator_Reset();

    Global_getMainWindow()->UpdateAllViews();
}

bool QEmulator::run(int frames)
{
    bool result = true;
    for (int i = 0; i < frames; i++)
    {
        int res = Emulator_SystemFrame();
        if (!res)
        {
            result = false;
            break;
        }

        if (i % 25 == 24)  // Update the screen every 25 frames
        {
            Global_getMainWindow()->UpdateAllViews();
            Global_getApplication()->processEvents();
            if (m_window->isAborted())
                return false;
        }
    }

    Global_getMainWindow()->UpdateAllViews();

    return result;
}

float QEmulator::getUptime()
{
    return Emulator_GetUptime();
}

void QEmulator::setBreakpoint(quint16 address)
{
    Emulator_SetCPUBreakpoint((WORD)address);
}
bool QEmulator::isBreakpoint()
{
    return Emulator_IsBreakpoint();
}

void QEmulator::saveScreenshot(const QString &filename)
{
    Global_getMainWindow()->saveScreenshot(filename);
}

bool QEmulator::attachCartridge(int slot, const QString & filename)
{
    //TODO: Check slot param
    return Global_getMainWindow()->attachCartridge(slot, filename);
}
void QEmulator::detachCartridge(int slot)
{
    //TODO: Check slot param
    Global_getMainWindow()->detachCartridge(slot);
}

bool QEmulator::attachFloppy(int slot, const QString & filename)
{
    //TODO: Check slot param
    return Global_getMainWindow()->attachFloppy(slot, filename);
}
void QEmulator::detachFloppy(int slot)
{
    //TODO: Check slot param
    Global_getMainWindow()->detachFloppy(slot);
}

void QEmulator::keyScan(uchar ukncscan, int timeout)
{
    g_pBoard->KeyboardEvent(ukncscan, TRUE);
    run(timeout);
    g_pBoard->KeyboardEvent(ukncscan, FALSE);
    run(3);
}

void QEmulator::keyScanShift(uchar ukncscan, int timeout)
{
    g_pBoard->KeyboardEvent(0105, TRUE);
    run(2);
    g_pBoard->KeyboardEvent(ukncscan, TRUE);
    run(timeout);
    g_pBoard->KeyboardEvent(ukncscan, FALSE);
    run(2);
    g_pBoard->KeyboardEvent(0105, FALSE);
    run(3);
}

const uchar arrChar2UkncScan[256] = {
/*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
/*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0153, 0000, 0000, 0153, 0000, 0000,
/*1*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*2*/    0113, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0117, 0175, 0146, 0173,
/*3*/    0176, 0030, 0031, 0032, 0013, 0034, 0035, 0016, 0017, 0177, 0174, 0007, 0000, 0000, 0000, 0000,
/*4*/    0077, 0072, 0076, 0050, 0057, 0033, 0047, 0055, 0156, 0073, 0027, 0052, 0056, 0112, 0054, 0075,
/*5*/    0053, 0067, 0074, 0111, 0114, 0051, 0137, 0071, 0115, 0070, 0157, 0036, 0136, 0037, 0110, 0155,
/*6*/    0126, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0005, 0146, 0131,
/*7*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};

const uchar arrChar2UkncScanShift[256] = {
/*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
/*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*1*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*2*/    0000, 0030, 0031, 0032, 0013, 0034, 0035, 0016, 0017, 0177, 0174, 0007, 0000, 0000, 0000, 0000,
/*3*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0117, 0175, 0135, 0173,
/*4*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*5*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*6*/    0077, 0072, 0076, 0050, 0057, 0033, 0047, 0055, 0156, 0073, 0027, 0052, 0056, 0112, 0054, 0075,
/*7*/    0053, 0067, 0074, 0111, 0114, 0051, 0137, 0071, 0115, 0070, 0157, 0036, 0136, 0037, 0000, 0000,
};

void QEmulator::keyChar(char ch, int timeout)
{
    if (ch < 0) return;  // ASCII only

    uchar scan = arrChar2UkncScanShift[(uchar)ch];
    if (scan != 0)
    {
        keyScanShift(scan, timeout);
        return;
    }

    scan = arrChar2UkncScan[(uchar)ch];
    if (scan == 0)
        return;
    keyScan(scan, timeout);
}

void QEmulator::keyString(QString str)
{
    for (int i = 0; i < str.length(); i++)
    {
        char ch = str[i].toAscii();
        keyChar(ch);
    }
}


//////////////////////////////////////////////////////////////////////
// QEmulatorProcessor

QEmulatorProcessor::QEmulatorProcessor(CProcessor* processor)
    : m_processor(processor)
{
}

QString QEmulatorProcessor::getName()
{
    return QString(m_processor->GetName());
}

ushort QEmulatorProcessor::getReg(int regno)
{
    //TODO: Check regno param
    return m_processor->GetReg(regno);
}
ushort QEmulatorProcessor::getPSW()
{
    return m_processor->GetPSW();
}

bool QEmulatorProcessor::isHalt()
{
    return m_processor->IsHaltMode();
}


//////////////////////////////////////////////////////////////////////
// QScriptWindow

QScriptWindow::QScriptWindow(QWidget * parent)
    : QDialog(parent, Qt::Dialog),
      m_aborted(false)
{
    setWindowTitle("Script Running");
    setMinimumSize(300, 125);
    setMaximumSize(400, 200);
    m_cancelButton.setText("Stop");
    m_vlayout.addWidget(&m_static, 0, 0);
    m_vlayout.addWidget(&m_cancelButton, 0, Qt::AlignHCenter);
    setLayout(&m_vlayout);
    setModal(true);

    QObject::connect(&m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonPressed()));

    m_emulator = new QEmulator(this);
    m_engine.globalObject().setProperty("emulator", m_engine.newQObject(m_emulator));
}

QScriptWindow::~QScriptWindow()
{
    delete m_emulator;
}

void QScriptWindow::runScript(const QString & script)
{
    show();
    raise();
    activateWindow();

    QString message;
    for (;;)
    {
        m_static.setText("Syntax check...");
        Global_getApplication()->processEvents();

        QScriptSyntaxCheckResult checkResult = QScriptEngine::checkSyntax(script);
        if (checkResult.state() != QScriptSyntaxCheckResult::Valid)
        {
            message.append("Syntax check FAILED:\n\n%1\n\nat line %2 column %3.")
                .arg(checkResult.errorMessage())
                .arg(checkResult.errorLineNumber())
                .arg(checkResult.errorColumnNumber());
            break;
        }

        m_static.setText("Running script...");
        Global_getApplication()->processEvents();

        m_engine.setProcessEventsInterval(250);
        QScriptValue result = m_engine.evaluate(script);
        if (m_aborted)
        {
            message.append("The script was STOPPED.");
            break;
        }

        message.append("The script FINISHED. The result is:\n\n")
            .append(result.toString());
        break;
    }

    m_static.setText(message);
    m_cancelButton.setText("Close");
    exec();
}

void QScriptWindow::cancelButtonPressed()
{
    if (m_engine.isEvaluating())
    {
        m_aborted = true;
        m_engine.abortEvaluation();
    }
    else  // The script is not running
    {
        QDialog::reject();  // Close the window
    }
}

void QScriptWindow::reject()
{
    if (m_engine.isEvaluating())
    {
        m_aborted = true;
        m_engine.abortEvaluation();
    }
    else  // The script is not running
    {
        QDialog::reject();
    }
}
