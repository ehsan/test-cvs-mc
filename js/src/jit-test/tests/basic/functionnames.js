/*
 * Most of these test cases are adapted from:
 * http://johnjbarton.github.com/nonymous/index.html
 */

function assertName(fn, name) {
    assertEq(displayName(fn), name)
}

/* simple names */
var a = function b() {};
function c() {};
assertName(a, 'b');
assertName(c, 'c');

var a = function(){},
    b = function(){};
assertName(a, 'a');
assertName(b, 'b');

/* nested names */
var main = function() {
    function Foo(a) { assertName(a, 'main/foo<') }
    var foo = new Foo(function() {});
};
assertName(main, 'main')
main();

/* duplicated */
var Baz = Bar = function(){}
assertName(Baz, 'Bar');
assertName(Bar, 'Bar');

/* returned from an immediate function */
var Foo = function (){
    assertName(arguments.callee, 'Foo<')
    return function(){};
}();
assertName(Foo, 'Foo');

/* various properties and such */
var x = {fox: { bax: function(){} } };
assertName(x.fox.bax, 'x.fox.bax');
var foo = {foo: {foo: {}}};
foo.foo.foo = function(){};
assertName(foo.foo.foo, 'foo.foo.foo');
var z = {
    foz: function() {
             var baz = function() {
                 var y = {bay: function() {}};
                 assertName(y.bay, 'z.foz/baz/y.bay');
             };
             assertName(baz, 'z.foz/baz');
             baz();
         }
};
assertName(z.foz, 'z.foz');
z.foz();

var outer = function() {
    x.fox.bax.nx = function(){};
    var w = {fow: { baw: function(){} } };
    assertName(x.fox.bax.nx, 'outer/x.fox.bax.nx')
    assertName(w.fow.baw, 'outer/w.fow.baw');
};
assertName(outer, 'outer');
outer();
function Fuz(){};
Fuz.prototype = {
  add: function() {}
}
assertName(Fuz.prototype.add, 'Fuz.add');

var x = 1;
x = function(){};
assertName(x, 'x');

var a = {b: {}};
a.b.c = (function() {
    assertName(arguments.callee, 'a.b.c<')
}());

a.b = function() {
    function foo(f) { assertName(f, 'a.b/<'); };
    return foo(function(){});
}
a.b();

var bar = 'bar';
a.b[bar] = function(){};
assertName(a.b.bar, 'a.b[bar]');

a.b = function() {
    assertName(arguments.callee, 'a.b<');
    return { a: function() {} }
}();
assertName(a.b.a, 'a.b.a');

a = {
    b: function(a) {
        if (a)
            return function() {};
        else
            return function() {};
    }
};
assertName(a.b, 'a.b');
assertName(a.b(true), 'a.b/<')
assertName(a.b(false), 'a.b/<')

function f(g) {
    assertName(g, 'x<');
    return g();
}
var x = f(function () { return function() {}; });
assertName(x, 'x</<');
