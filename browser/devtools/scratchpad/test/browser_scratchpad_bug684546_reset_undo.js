/* vim: set ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

// Reference to the Scratchpad chrome window object.
let gScratchpadWindow;

// Reference to the Scratchpad object.
let gScratchpad;

// Reference to the temporary nsIFile we will work with.
let gFileA;
let gFileB;

// The temporary file content.
let gFileAContent = "// File A ** Hello World!";
let gFileBContent = "// File B ** Goodbye All";

// Help track if one or both files are saved
let gFirstFileSaved = false;

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function browserLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", browserLoad, true);
    openScratchpad(runTests);
  }, true);

  content.location = "data:text/html,<p>test that undo get's reset after file load in Scratchpad";
}

function runTests()
{
  gScratchpad = gScratchpadWindow.Scratchpad;

  // Create a temporary file.
  gFileA = FileUtils.getFile("TmpD", ["fileAForBug684546.tmp"]);
  gFileA.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  gFileB = FileUtils.getFile("TmpD", ["fileBForBug684546.tmp"]);
  gFileB.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  // Write the temporary file.
  let foutA = Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(Ci.nsIFileOutputStream);
  foutA.init(gFileA.QueryInterface(Ci.nsILocalFile), 0x02 | 0x08 | 0x20,
            0644, foutA.DEFER_OPEN);

  let foutB = Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(Ci.nsIFileOutputStream);
  foutB.init(gFileB.QueryInterface(Ci.nsILocalFile), 0x02 | 0x08 | 0x20,
            0644, foutB.DEFER_OPEN);

  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let fileContentStreamA = converter.convertToInputStream(gFileAContent);
  let fileContentStreamB = converter.convertToInputStream(gFileBContent);

  NetUtil.asyncCopy(fileContentStreamA, foutA, tempFileSaved);
  NetUtil.asyncCopy(fileContentStreamB, foutB, tempFileSaved);
}

function tempFileSaved(aStatus)
{
  let success = Components.isSuccessCode(aStatus);

  ok(success, "a temporary file was saved successfully");

  if (!success)
  {
    finish();
    return;
  }

  if (gFirstFileSaved && success)
  {
    ok((gFirstFileSaved && success), "Both files loaded");
    // Import the file A into Scratchpad.
    gScratchpad.importFromFile(gFileA.QueryInterface(Ci.nsILocalFile),  true,
                              fileAImported);
  }
  gFirstFileSaved = success;
}

function fileAImported(aStatus, aFileContent)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file A was imported successfully with Scratchpad");

  is(aFileContent, gFileAContent, "received data is correct");

  is(gScratchpad.getText(), gFileAContent, "the editor content is correct");

  gScratchpad.setText("new text", gScratchpad.getText().length);

  is(gScratchpad.getText(), gFileAContent + "new text", "text updated correctly");
  gScratchpad.undo();
  is(gScratchpad.getText(), gFileAContent, "undo works");
  gScratchpad.redo();
  is(gScratchpad.getText(), gFileAContent + "new text", "redo works");

  // Import the file B into Scratchpad.
  gScratchpad.importFromFile(gFileB.QueryInterface(Ci.nsILocalFile),  true,
                            fileBImported);
}

function fileBImported(aStatus, aFileContent)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file B was imported successfully with Scratchpad");

  is(aFileContent, gFileBContent, "received data is correct");

  is(gScratchpad.getText(), gFileBContent, "the editor content is correct");

  ok(!gScratchpad.editor.canUndo(), "editor cannot undo after load");

  gScratchpad.undo();
  is(gScratchpad.getText(), gFileBContent,
      "the editor content is still correct after undo");

  gScratchpad.setText("new text", gScratchpad.getText().length);
  is(gScratchpad.getText(), gFileBContent + "new text", "text updated correctly");

  gScratchpad.undo();
  is(gScratchpad.getText(), gFileBContent, "undo works");
  ok(!gScratchpad.editor.canUndo(), "editor cannot undo after load (again)");

  gScratchpad.redo();
  is(gScratchpad.getText(), gFileBContent + "new text", "redo works");

  // Done!
  finish();
}

registerCleanupFunction(function() {
  if (gFileA && gFileA.exists())
  {
    gFileA.remove(false);
    gFileA = null;
  }
  if (gFileB && gFileB.exists())
  {
    gFileB.remove(false);
    gFileB = null;
  }
  gScratchpad = null;
});
