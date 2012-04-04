/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

/* If charset parameter is invalid, the encoding should be detected as UTF-8 */

function run_test()
{
  let body = '<?xml version="1.0"><html>%c3%80</html>';
  let result = '<?xml version="1.0"><html>\u00c0</html>';

  let xhr = Components.classes['@mozilla.org/xmlextras/xmlhttprequest;1'].
            createInstance(Components.interfaces.nsIXMLHttpRequest);
  xhr.open('GET',
           'data:text/xml;charset=abc,' + body,
           false);
  xhr.send(null);

  do_check_eq(xhr.responseText, result);
}
