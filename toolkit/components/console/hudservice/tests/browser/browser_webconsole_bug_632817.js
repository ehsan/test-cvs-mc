/* vim:set ts=2 sw=2 sts=2 et: */
/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

// Tests that network log messages bring up the network panel.

const TEST_NETWORK_REQUEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-network-request.html";

const TEST_IMG = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-image.png";

const TEST_DATA_JSON_CONTENT =
  '{ id: "test JSON data", myArray: [ "foo", "bar", "baz", "biff" ] }';

let lastRequest = null;
let requestCallback = null;

function test()
{
  addTab("data:text/html,Web Console network logging tests");

  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    openConsole();
    is(HUDService.displaysIndex().length, 1, "Web Console was opened");

    hudId = HUDService.displaysIndex()[0];
    hud = HUDService.getHeadsUpDisplay(hudId);

    HUDService.lastFinishedRequestCallback = function(aRequest) {
      lastRequest = aRequest;
      if (requestCallback) {
        requestCallback();
      }
    };

    executeSoon(testPageLoad);
  }, true);
}

function testPageLoad()
{
  browser.addEventListener("load", function(aEvent) {
    browser.removeEventListener(aEvent.type, arguments.callee, true);

    // Check if page load was logged correctly.
    ok(lastRequest, "Page load was logged");
    is(lastRequest.url, TEST_NETWORK_REQUEST_URI,
      "Logged network entry is page load");
    is(lastRequest.method, "GET", "Method is correct");

    lastRequest = null;
    executeSoon(testPageLoadBody);
  }, true);

  content.location = TEST_NETWORK_REQUEST_URI;
}

function testPageLoadBody()
{
  // Turn on logging of request bodies and check again.
  HUDService.saveRequestAndResponseBodies = true;
  browser.addEventListener("load", function(aEvent) {
    browser.removeEventListener(aEvent.type, arguments.callee, true);

    ok(lastRequest, "Page load was logged again");
    is(lastRequest.response.body.indexOf("<!DOCTYPE HTML>"), 0,
      "Response body's beginning is okay");

    lastRequest = null;
    executeSoon(testXhrGet);
  }, true);

  content.location.reload();
}

function testXhrGet()
{
  requestCallback = function() {
    ok(lastRequest, "testXhrGet() was logged");
    is(lastRequest.method, "GET", "Method is correct");
    is(lastRequest.request.body, null, "No request body was sent");
    is(lastRequest.response.body, TEST_DATA_JSON_CONTENT,
      "Response is correct");

    lastRequest = null;
    requestCallback = null;
    executeSoon(testXhrPost);
  };

  // Start the XMLHttpRequest() GET test.
  content.wrappedJSObject.testXhrGet();
}

function testXhrPost()
{
  requestCallback = function() {
    ok(lastRequest, "testXhrPost() was logged");
    is(lastRequest.method, "POST", "Method is correct");
    is(lastRequest.request.body, "Hello world!",
      "Request body was logged");
    is(lastRequest.response.body, TEST_DATA_JSON_CONTENT,
      "Response is correct");

    lastRequest = null;
    requestCallback = null;
    executeSoon(testFormSubmission);
  };

  // Start the XMLHttpRequest() POST test.
  content.wrappedJSObject.testXhrPost();
}

function testFormSubmission()
{
  // Start the form submission test. As the form is submitted, the page is
  // loaded again. Bind to the load event to catch when this is done.
  browser.addEventListener("load", function(aEvent) {
    browser.removeEventListener(aEvent.type, arguments.callee, true);

    ok(lastRequest, "testFormSubmission() was logged");
    is(lastRequest.method, "POST", "Method is correct");
    isnot(lastRequest.request.body.
      indexOf("Content-Type: application/x-www-form-urlencoded"), -1,
      "Content-Type is correct");
    isnot(lastRequest.request.body.
      indexOf("Content-Length: 20"), -1, "Content-length is correct");
    isnot(lastRequest.request.body.
      indexOf("name=foo+bar&age=144"), -1, "Form data is correct");
    ok(lastRequest.response.body.indexOf("<!DOCTYPE HTML>") == 0,
      "Response body's beginning is okay");

    executeSoon(testLiveFilteringOnSearchStrings);
  }, true);

  let form = content.document.querySelector("form");
  ok(form, "we have the HTML form");
  form.submit();
}

function testLiveFilteringOnSearchStrings() {
  browser.removeEventListener("DOMContentLoaded",
                              testLiveFilteringOnSearchStrings, false);

  setStringFilter("http");
  isnot(countMessageNodes(), 0, "the log nodes are not hidden when the " +
    "search string is set to \"http\"");

  setStringFilter("HTTP");
  isnot(countMessageNodes(), 0, "the log nodes are not hidden when the " +
    "search string is set to \"HTTP\"");

  setStringFilter("hxxp");
  is(countMessageNodes(), 0, "the log nodes are hidden when the search " +
    "string is set to \"hxxp\"");

  setStringFilter("ht tp");
  isnot(countMessageNodes(), 0, "the log nodes are not hidden when the " +
    "search string is set to \"ht tp\"");

  setStringFilter("");
  isnot(countMessageNodes(), 0, "the log nodes are not hidden when the " +
    "search string is removed");

  setStringFilter("json");
  is(countMessageNodes(), 2, "the log nodes show only the nodes with \"json\"");

  setStringFilter("'foo'");
  is(countMessageNodes(), 0, "the log nodes are hidden when searching for " +
    "the string 'foo'");

  setStringFilter("foo\"bar'baz\"boo'");
  is(countMessageNodes(), 0, "the log nodes are hidden when searching for " +
    "the string \"foo\"bar'baz\"boo'\"");

  finishTest();
}

function countMessageNodes() {
  let outputNode = hud.querySelector(".hud-output-node");

  let messageNodes = outputNode.querySelectorAll(".hud-msg-node");
  let displayedMessageNodes = 0;
  let view = outputNode.ownerDocument.defaultView;
  for (let i = 0; i < messageNodes.length; i++) {
    let computedStyle = view.getComputedStyle(messageNodes[i], null);
    if (computedStyle.display !== "none")
      displayedMessageNodes++;
  }

  return displayedMessageNodes;
}

function setStringFilter(aValue)
{
  hud.querySelector(".hud-filter-box").value = aValue;
  HUDService.adjustVisibilityOnSearchStringChange(hudId, aValue);
}
