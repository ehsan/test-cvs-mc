# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# Contributor(s):
#   Chris Jones <jones.chris.g@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either of the GNU General Public License Version 2 or later (the "GPL"),
# or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

import os, sys
from ply import lex, yacc

from ipdl.ast import *

def _getcallerpath():
    '''Return the absolute path of the file containing the code that
**CALLED** this function.'''
    return os.path.abspath(sys._getframe(1).f_code.co_filename)

##-----------------------------------------------------------------------------

class ParseError(Exception):
    def __init__(self, loc, fmt, *args):
        self.loc = loc
        self.error = ('%s%s: error: %s'% (
            Parser.includeStackString(), loc, fmt)) % args
    def __str__(self):
        return self.error

def _safeLinenoValue(t):
    lineno, value = 0, '???'
    if hasattr(t, 'lineno'): lineno = t.lineno
    if hasattr(t, 'value'):  value = t.value
    return lineno, value

def _error(loc, fmt, *args):
    raise ParseError(loc, fmt, *args)

class Parser:
    # when we reach an |include protocol "foo.ipdl";| statement, we need to
    # save the current parser state and create a new one.  this "stack" is
    # where that state is saved
    #
    # there is one Parser per file
    current = None
    parseStack = [ ]
    parsed = { }

    def __init__(self, debug=0):
        self.debug = debug
        self.filename = None
        self.includedirs = None
        self.loc = None         # not always up to date
        self.lexer = None
        self.parser = None
        self.tu = TranslationUnit()
        self.direction = None
        self.errout = None

    def parse(self, input, filename, includedirs, errout):
        assert os.path.isabs(filename)

        if filename in Parser.parsed:
            return Parser.parsed[filename].tu

        self.lexer = lex.lex(debug=self.debug,
                             optimize=not self.debug,
                             lextab="ipdl_lextab")
        self.parser = yacc.yacc(debug=self.debug,
                                optimize=not self.debug,
                                tabmodule="ipdl_yacctab")
        self.filename = filename
        self.includedirs = includedirs
        self.tu.filename = filename
        self.errout = errout

        Parser.parsed[filename] = self
        Parser.parseStack.append(Parser.current)
        Parser.current = self

        try:
            ast = self.parser.parse(input=input, lexer=self.lexer,
                                    debug=self.debug)
        except ParseError, p:
            print >>errout, p
            return None

        Parser.current = Parser.parseStack.pop()
        return ast

    def resolveIncludePath(self, filepath):
        '''Return the absolute path from which the possibly partial
|filepath| should be read, or |None| if |filepath| cannot be located.'''
        for incdir in self.includedirs +[ '' ]:
            realpath = os.path.join(incdir, filepath)
            if os.path.isfile(realpath):
                return os.path.abspath(realpath)
        return None

    # returns a GCC-style string representation of the include stack.
    # e.g.,
    #   in file included from 'foo.ipdl', line 120:
    #   in file included from 'bar.ipd', line 12:
    # which can be printed above a proper error message or warning
    @staticmethod
    def includeStackString():
        s = ''
        for parse in Parser.parseStack[1:]:
            s += "  in file included from `%s', line %d:\n"% (
                parse.loc.filename, parse.loc.lineno)
        return s

def locFromTok(p, num):
    return Loc(Parser.current.filename, p.lineno(num))


##-----------------------------------------------------------------------------

reserved = set((
        'answer',
        'as',
        'async',
        'both',
        'bridges',
        'call',
        'child',
        '__delete__',
        'delete',                       # reserve 'delete' to prevent its use
        'goto',
        'include',
        'manager',
        'manages',
        'namespace',
        'nullable',
        'or',
        'parent',
        'protocol',
        'recv',
        'returns',
        'rpc',
        'send',
        'spawns',
        'start',
        'state',
        'sync',
        'union',
        'using'))
tokens = [
    'COLONCOLON', 'ID', 'STRING'
] + [ r.upper() for r in reserved ]

t_COLONCOLON = '::'

literals = '(){}[]<>;:,~'
t_ignore = ' \f\t\v'

def t_linecomment(t):
    r'//[^\n]*'

def t_multilinecomment(t):
    r'/\*(\n|.)*?\*/'
    t.lexer.lineno += t.value.count('\n')

def t_NL(t):
    r'(?:\r\n|\n|\n)+'
    t.lexer.lineno += len(t.value)

def t_ID(t):
    r'[a-zA-Z_][a-zA-Z0-9_]*'
    if t.value in reserved:
        t.type = t.value.upper()
    return t

def t_STRING(t):
    r'"[^"\n]*"'
    t.value = t.value[1:-1]
    return t

def t_error(t):
    _error(Loc(Parser.current.filename, t.lineno),
           'lexically invalid characters `%s', t.value)

##-----------------------------------------------------------------------------

def p_TranslationUnit(p):
    """TranslationUnit : Preamble NamespacedStuff"""
    tu = Parser.current.tu
    for stmt in p[1]:
        if isinstance(stmt, CxxInclude):
            tu.addCxxInclude(stmt)
        elif isinstance(stmt, ProtocolInclude):
            tu.addProtocolInclude(stmt)
        elif isinstance(stmt, UsingStmt):
            tu.addUsingStmt(stmt)
        else:
            assert 0

    for thing in p[2]:
        if isinstance(thing, UnionDecl):
            tu.addUnionDecl(thing)
        elif isinstance(thing, Protocol):
            if tu.protocol is not None:
                _error(thing.loc, "only one protocol definition per file")
            tu.protocol = thing
        else:
            assert(0)

    p[0] = tu

##--------------------
## Preamble
def p_Preamble(p):
    """Preamble : Preamble PreambleStmt ';'
                |"""
    if 1 == len(p):
        p[0] = [ ]
    else:
        p[1].append(p[2])
        p[0] = p[1]

def p_PreambleStmt(p):
    """PreambleStmt : CxxIncludeStmt
                    | ProtocolIncludeStmt
                    | UsingStmt"""
    p[0] = p[1]

def p_CxxIncludeStmt(p):
    """CxxIncludeStmt : INCLUDE STRING"""
    p[0] = CxxInclude(locFromTok(p, 1), p[2])

def p_ProtocolIncludeStmt(p):
    """ProtocolIncludeStmt : INCLUDE PROTOCOL ID
                           | INCLUDE PROTOCOL STRING"""
    loc = locFromTok(p, 1)
    
    if 0 <= p[3].rfind('.ipdl'):
        _error(loc, "`include protocol \"P.ipdl\"' syntax is obsolete.  Use `include protocol P' instead.")
    
    Parser.current.loc = loc
    inc = ProtocolInclude(loc, p[3])

    path = Parser.current.resolveIncludePath(inc.file)
    if path is None:
        raise ParseError(loc, "can't locate protocol include file `%s'"% (
                inc.file))
    
    inc.tu = Parser().parse(open(path).read(), path, Parser.current.includedirs, Parser.current.errout)
    p[0] = inc

def p_UsingStmt(p):
    """UsingStmt : USING CxxType"""
    p[0] = UsingStmt(locFromTok(p, 1), p[2])

##--------------------
## Namespaced stuff
def p_NamespacedStuff(p):
    """NamespacedStuff : NamespacedStuff NamespaceThing
                       | NamespaceThing"""
    if 2 == len(p):
        p[0] = p[1]
    else:
        p[1].extend(p[2])
        p[0] = p[1]

def p_NamespaceThing(p):
    """NamespaceThing : NAMESPACE ID '{' NamespacedStuff '}'
                      | UnionDecl
                      | ProtocolDefn"""
    if 2 == len(p):
        p[0] = [ p[1] ]
    else:
        for thing in p[4]:
            thing.addOuterNamespace(Namespace(locFromTok(p, 1), p[2]))
        p[0] = p[4]
    
def p_UnionDecl(p):
    """UnionDecl : UNION ID '{' ComponentTypes  '}' ';'"""
    p[0] = UnionDecl(locFromTok(p, 1), p[2], p[4])

def p_ComponentTypes(p):
    """ComponentTypes : ComponentTypes Type ';'
                      | Type ';'"""
    if 3 == len(p):
        p[0] = [ p[1] ]
    else:
        p[1].append(p[2])
        p[0] = p[1]

def p_ProtocolDefn(p):
    """ProtocolDefn : OptionalSendSemanticsQual PROTOCOL ID '{' ProtocolBody '}' ';'"""
    protocol = p[5]
    protocol.loc = locFromTok(p, 2)
    protocol.name = p[3]
    protocol.sendSemantics = p[1]
    p[0] = protocol

def p_ProtocolBody(p):
    """ProtocolBody : SpawnsStmtsOpt"""
    p[0] = p[1]

##--------------------
## spawns/bridges stmts

def p_SpawnsStmtsOpt(p):
    """SpawnsStmtsOpt : SpawnsStmt SpawnsStmtsOpt
                      | BridgesStmtsOpt"""
    if 2 == len(p):
        p[0] = p[1]
    else:
        p[2].spawnsStmts.insert(0, p[1])
        p[0] = p[2]

def p_SpawnsStmt(p):
    """SpawnsStmt : PARENT SPAWNS ID AsOpt ';'
                  | CHILD SPAWNS ID AsOpt ';'"""
    p[0] = SpawnsStmt(locFromTok(p, 1), p[1], p[3], p[4])

def p_AsOpt(p):
    """AsOpt : AS PARENT
             | AS CHILD
             | """
    if 3 == len(p):
        p[0] = p[2]
    else:
        p[0] = 'child'

def p_BridgesStmtsOpt(p):
    """BridgesStmtsOpt : BridgesStmt BridgesStmtsOpt
                       | ManagersStmtOpt"""
    if 2 == len(p):
        p[0] = p[1]
    else:
        p[2].bridgesStmts.insert(0, p[1])
        p[0] = p[2]

def p_BridgesStmt(p):
    """BridgesStmt : BRIDGES ID ',' ID ';'"""
    p[0] = BridgesStmt(locFromTok(p, 1), p[2], p[4])

##--------------------
## manager/manages stmts

def p_ManagersStmtOpt(p):
    """ManagersStmtOpt : ManagersStmt ManagesStmtsOpt
                       | ManagesStmtsOpt"""
    if 2 == len(p):
        p[0] = p[1]
    else:
        p[2].managers = p[1]
        p[0] = p[2]

def p_ManagersStmt(p):
    """ManagersStmt : MANAGER ManagerList ';'"""
    if 1 == len(p):
        p[0] = [ ]
    else:
        p[0] = p[2]

def p_ManagerList(p):
    """ManagerList : ID
                   | ManagerList OR ID"""
    if 2 == len(p):
        p[0] = [ Manager(locFromTok(p, 1), p[1]) ]
    else:
        p[1].append(Manager(locFromTok(p, 3), p[3]))
        p[0] = p[1]

def p_ManagesStmtsOpt(p):
    """ManagesStmtsOpt : ManagesStmt ManagesStmtsOpt
                       | MessageDeclsOpt"""
    if 2 == len(p):
        p[0] = p[1]
    else:
        p[2].managesStmts.insert(0, p[1])
        p[0] = p[2]

def p_ManagesStmt(p):
    """ManagesStmt : MANAGES ID ';'"""
    p[0] = ManagesStmt(locFromTok(p, 1), p[2])


##--------------------
## Message decls

def p_MessageDeclsOpt(p):
    """MessageDeclsOpt : MessageDeclThing MessageDeclsOpt
                       | TransitionStmtsOpt"""
    if 2 == len(p):
        p[0] = p[1]
    else:
        p[2].messageDecls.insert(0, p[1])
        p[0] = p[2]

def p_MessageDeclThing(p):
    """MessageDeclThing : MessageDirectionLabel ':' MessageDecl ';'
                        | MessageDecl ';'"""
    if 3 == len(p):
        p[0] = p[1]
    else:
        p[0] = p[3]

def p_MessageDirectionLabel(p):
    """MessageDirectionLabel : PARENT
                             | CHILD
                             | BOTH"""
    if p[1] == 'parent':
        Parser.current.direction = IN
    elif p[1] == 'child':
        Parser.current.direction = OUT
    elif p[1] == 'both':
        Parser.current.direction = INOUT
    else:
        assert 0

def p_MessageDecl(p):
    """MessageDecl : OptionalSendSemanticsQual MessageBody"""
    msg = p[2]
    msg.sendSemantics = p[1]

    if Parser.current.direction is None:
        _error(msg.loc, 'missing message direction')
    msg.direction = Parser.current.direction

    p[0] = msg

def p_MessageBody(p):
    """MessageBody : MessageId MessageInParams MessageOutParams"""
    # FIXME/cjones: need better loc info: use one of the quals
    loc, name = p[1]
    msg = MessageDecl(loc)
    msg.name = name
    msg.addInParams(p[2])
    msg.addOutParams(p[3])
    p[0] = msg

def p_MessageId(p):
    """MessageId : ID
                 | __DELETE__
                 | DELETE
                 | '~' ID"""
    loc = locFromTok(p, 1)
    if 3 == len(p):
        _error(loc, "sorry, `%s()' destructor syntax is a relic from a bygone era.  Declare `__delete__()' in the `%s' protocol instead", p[1]+p[2], p[2])
    elif 'delete' == p[1]:
        _error(loc, "`delete' is a reserved identifier")
    p[0] = [ loc, p[1] ]

def p_MessageInParams(p):
    """MessageInParams : '(' ParamList ')'"""
    p[0] = p[2]

def p_MessageOutParams(p):
    """MessageOutParams : RETURNS '(' ParamList ')'
                        | """
    if 1 == len(p):
        p[0] = [ ]
    else:
        p[0] = p[3]

##--------------------
## State machine

def p_TransitionStmtsOpt(p):
    """TransitionStmtsOpt : TransitionStmt TransitionStmtsOpt
                          |"""
    if 1 == len(p):
        # we fill in |loc| in the Protocol rule
        p[0] = Protocol(None)
    else:
        p[2].transitionStmts.insert(0, p[1])
        p[0] = p[2]

def p_TransitionStmt(p):
    """TransitionStmt : OptionalStart STATE State ':' Transitions"""
    p[3].start = p[1]
    p[0] = TransitionStmt(locFromTok(p, 2), p[3], p[5])

def p_OptionalStart(p):
    """OptionalStart : START
                     | """
    p[0] = (len(p) == 2)                # True iff 'start' specified

def p_Transitions(p):
    """Transitions : Transitions Transition
                   | Transition"""
    if 3 == len(p):
        p[1].append(p[2])
        p[0] = p[1]
    else:
        p[0] = [ p[1] ]

def p_Transition(p):
    """Transition : Trigger ID GOTO StateList ';'
                  | Trigger __DELETE__ ';'
                  | Trigger DELETE ';'"""
    if 'delete' == p[2]:
        _error(locFromTok(p, 1), "`delete' is a reserved identifier")
    
    loc, trigger = p[1]
    if 6 == len(p):
        nextstates = p[4]
    else:
        nextstates = [ State.DEAD ]
    p[0] = Transition(loc, trigger, p[2], nextstates)

def p_Trigger(p):
    """Trigger : SEND
               | RECV
               | CALL
               | ANSWER"""
    p[0] = [ locFromTok(p, 1), Transition.nameToTrigger(p[1]) ]

def p_StateList(p):
    """StateList : StateList OR State
                 | State"""
    if 2 == len(p):
        p[0] = [ p[1] ]
    else:
        p[1].append(p[3])
        p[0] = p[1]

def p_State(p):
    """State : ID"""
    p[0] = State(locFromTok(p, 1), p[1])

##--------------------
## Minor stuff
def p_OptionalSendSemanticsQual(p):
    """OptionalSendSemanticsQual : SendSemanticsQual
                                 | """
    if 2 == len(p): p[0] = p[1]
    else:           p[0] = ASYNC

def p_SendSemanticsQual(p):
    """SendSemanticsQual : ASYNC
                         | RPC
                         | SYNC"""
    s = p[1]
    if 'async' == s: p[0] = ASYNC
    elif 'rpc' == s: p[0] = RPC
    elif 'sync'== s: p[0] = SYNC
    else:
        assert 0

def p_ParamList(p):
    """ParamList : ParamList ',' Param
                 | Param
                 | """
    if 1 == len(p):
        p[0] = [ ]
    elif 2 == len(p):
        p[0] = [ p[1] ]
    else:
        p[1].append(p[3])
        p[0] = p[1]

def p_Param(p):
    """Param : Type ID"""
    p[0] = Param(locFromTok(p, 1), p[1], p[2])

def p_Type(p):
    """Type : MaybeNullable BasicType"""
    # only actor types are nullable; we check this in the type checker
    p[2].nullable = p[1]
    p[0] = p[2]

def p_BasicType(p):
    """BasicType : ScalarType
                 | ScalarType '[' ']'"""
    if 4 == len(p):
        p[1].array = 1
    p[0] = p[1]

def p_ScalarType(p):
    """ScalarType : ActorType
                  | CxxID"""    # ID == CxxType; we forbid qnames here,
                                # in favor of the |using| declaration
    if isinstance(p[1], TypeSpec):
        p[0] = p[1]
    else:
        loc, id = p[1]
        p[0] = TypeSpec(loc, QualifiedId(loc, id))

def p_ActorType(p):
    """ActorType : ID ':' State"""
    loc = locFromTok(p, 1)
    p[0] = TypeSpec(loc, QualifiedId(loc, p[1]), state=p[3])
 
def p_MaybeNullable(p):
    """MaybeNullable : NULLABLE
                     | """
    p[0] = (2 == len(p))

##--------------------
## C++ stuff
def p_CxxType(p):
    """CxxType : QualifiedID
               | CxxID"""
    if isinstance(p[1], QualifiedId):
        p[0] = TypeSpec(p[1].loc, p[1])
    else:
        loc, id = p[1]
        p[0] = TypeSpec(loc, QualifiedId(loc, id))

def p_QualifiedID(p):
    """QualifiedID : QualifiedID COLONCOLON CxxID
                   | CxxID COLONCOLON CxxID"""
    if isinstance(p[1], QualifiedId):
        loc, id = p[3]
        p[1].qualify(id)
        p[0] = p[1]
    else:
        loc1, id1 = p[1]
        _, id2 = p[3]
        p[0] = QualifiedId(loc1, id2, [ id1 ])

def p_CxxID(p):
    """CxxID : ID
             | CxxTemplateInst"""
    if isinstance(p[1], tuple):
        p[0] = p[1]
    else:
        p[0] = (locFromTok(p, 1), str(p[1]))

def p_CxxTemplateInst(p):
    """CxxTemplateInst : ID '<' ID '>'"""
    p[0] = (locFromTok(p, 1), str(p[1]) +'<'+ str(p[3]) +'>')

def p_error(t):
    lineno, value = _safeLinenoValue(t)
    _error(Loc(Parser.current.filename, lineno),
           "bad syntax near `%s'", value)
