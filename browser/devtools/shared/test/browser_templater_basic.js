/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

// Tests that the DOM Template engine works properly

Cu.import("resource:///modules/devtools/Templater.jsm");
Cu.import("resource:///modules/devtools/Promise.jsm");

function test() {
  addTab("http://example.com/browser/browser/devtools/shared/test/browser_templater_basic.html", function() {
    info("Starting DOM Templater Tests");
    runTest(0);
  });
}

function runTest(index) {
  var options = tests[index] = tests[index]();
  var holder = content.document.createElement('div');
  holder.id = options.name;
  var body = content.document.body;
  body.appendChild(holder);
  holder.innerHTML = options.template;

  info('Running ' + options.name);
  new Templater().processNode(holder, options.data);

  if (typeof options.result == 'string') {
    is(holder.innerHTML, options.result, options.name);
  }
  else {
    ok(holder.innerHTML.match(options.result), options.name);
  }

  if (options.also) {
    options.also(options);
  }

  function runNextTest() {
    index++;
    if (index < tests.length) {
      runTest(index);
    }
    else {
      finished();
    }
  }

  if (options.later) {
    var ais = is.bind(this);

    function createTester(holder, options) {
      return function() {
        ais(holder.innerHTML, options.later, options.name + ' later');
        runNextTest();
      }.bind(this);
    }

    executeSoon(createTester(holder, options));
  }
  else {
    runNextTest();
  }
}

function finished() {
  gBrowser.removeCurrentTab();
  info("Finishing DOM Templater Tests");
  tests = null;
  finish();
}

/**
 * Why have an array of functions that return data rather than just an array
 * of the data itself? Some of these tests contain calls to delayReply() which
 * sets up async processing using executeSoon(). Since the execution of these
 * tests is asynchronous, the delayed reply will probably arrive before the
 * test is executed, making the test be synchronous. So we wrap the data in a
 * function so we only set it up just before we use it.
 */
var tests = [
  function() { return {
    name: 'simpleNesting',
    template: '<div id="ex1">${nested.value}</div>',
    data: { nested:{ value:'pass 1' } },
    result: '<div id="ex1">pass 1</div>'
  };},

  function() { return {
    name: 'returnDom',
    template: '<div id="ex2">${__element.ownerDocument.createTextNode(\'pass 2\')}</div>',
    data: {},
    result: '<div id="ex2">pass 2</div>'
  };},

  function() { return {
    name: 'srcChange',
    template: '<img _src="${fred}" id="ex3">',
    data: { fred:'green.png' },
    result: /<img( id="ex3")? src="green.png"( id="ex3")?>/
  };},

  function() { return {
    name: 'ifTrue',
    template: '<p if="${name !== \'jim\'}">hello ${name}</p>',
    data: { name: 'fred' },
    result: '<p>hello fred</p>'
  };},

  function() { return {
    name: 'ifFalse',
    template: '<p if="${name !== \'jim\'}">hello ${name}</p>',
    data: { name: 'jim' },
    result: ''
  };},

  function() { return {
    name: 'simpleLoop',
    template: '<p foreach="index in ${[ 1, 2, 3 ]}">${index}</p>',
    data: {},
    result: '<p>1</p><p>2</p><p>3</p>'
  };},

  function() { return {
    name: 'loopElement',
    template: '<loop foreach="i in ${array}">${i}</loop>',
    data: { array: [ 1, 2, 3 ] },
    result: '123'
  };},

  // Bug 692031: DOMTemplate async loops do not drop the loop element
  function() { return {
    name: 'asyncLoopElement',
    template: '<loop foreach="i in ${array}">${i}</loop>',
    data: { array: delayReply([1, 2, 3]) },
    result: '<span></span>',
    later: '123'
  };},

  function() { return {
    name: 'saveElement',
    template: '<p save="${element}">${name}</p>',
    data: { name: 'pass 8' },
    result: '<p>pass 8</p>',
    also: function(options) {
      ok(options.data.element.innerHTML, 'pass 9', 'saveElement saved');
      delete options.data.element;
    }
  };},

  function() { return {
    name: 'useElement',
    template: '<p id="pass9">${adjust(__element)}</p>',
    data: {
      adjust: function(element) {
        is('pass9', element.id, 'useElement adjust');
        return 'pass 9b'
      }
    },
    result: '<p id="pass9">pass 9b</p>'
  };},

  function() { return {
    name: 'asyncInline',
    template: '${delayed}',
    data: { delayed: delayReply('inline') },
    result: '<span></span>',
    later: 'inline'
  };},

  function() { return {
    name: 'asyncArray',
    template: '<p foreach="i in ${delayed}">${i}</p>',
    data: { delayed: delayReply([1, 2, 3]) },
    result: '<span></span>',
    later: '<p>1</p><p>2</p><p>3</p>'
  };},

  function() { return {
    name: 'asyncMember',
    template: '<p foreach="i in ${delayed}">${i}</p>',
    data: { delayed: [delayReply(4), delayReply(5), delayReply(6)] },
    result: '<span></span><span></span><span></span>',
    later: '<p>4</p><p>5</p><p>6</p>'
  };},

  function() { return {
    name: 'asyncBoth',
    template: '<p foreach="i in ${delayed}">${i}</p>',
    data: {
      delayed: delayReply([
        delayReply(4),
        delayReply(5),
        delayReply(6)
      ])
    },
    result: '<span></span>',
    later: '<p>4</p><p>5</p><p>6</p>'
  };}
];

function delayReply(data) {
  var p = new Promise();
  executeSoon(function() {
    p.resolve(data);
  });
  return p;
}
