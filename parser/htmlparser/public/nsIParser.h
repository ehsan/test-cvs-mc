/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=2 sw=2 et tw=78: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef NS_IPARSER___
#define NS_IPARSER___


 /**
 * This GECKO-INTERNAL interface is on track to being REMOVED (or refactored
 * to the point of being near-unrecognizable).
 *
 * Please DO NOT #include this file in comm-central code, in your XULRunner
 * app or binary extensions.
 *
 * Please DO NOT #include this into new files even inside Gecko. It is more
 * likely than not that #including this header is the wrong thing to do.
 */

#include "nsISupports.h"
#include "nsIStreamListener.h"
#include "nsIDTD.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsIAtom.h"
#include "nsParserBase.h"

#define NS_IPARSER_IID \
{ 0x2c4ad90a, 0x740e, 0x4212, \
  { 0xba, 0x3f, 0xfe, 0xac, 0xda, 0x4b, 0x92, 0x9e } }

// {41421C60-310A-11d4-816F-000064657374}
#define NS_IDEBUG_DUMP_CONTENT_IID \
{ 0x41421c60, 0x310a, 0x11d4, \
{ 0x81, 0x6f, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

class nsIContentSink;
class nsIRequestObserver;
class nsString;
class nsIURI;
class nsIChannel;
class nsIContent;

enum eParserCommands {
  eViewNormal,
  eViewSource,
  eViewFragment,
  eViewErrors
};

enum eParserDocType {
  ePlainText = 0,
  eXML,
  eHTML_Quirks,
  eHTML_Strict
};

enum eStreamState {eNone,eOnStart,eOnDataAvail,eOnStop};

/** 
 *  FOR DEBUG PURPOSE ONLY
 *
 *  Use this interface to query objects that contain content information.
 *  Ex. Parser can trigger dump content by querying the sink that has
 *      access to the content.
 *  
 *  @update  harishd 05/25/00
 */
class nsIDebugDumpContent : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDEBUG_DUMP_CONTENT_IID)
  NS_IMETHOD DumpContentModel()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDebugDumpContent, NS_IDEBUG_DUMP_CONTENT_IID)

/**
 * This GECKO-INTERNAL interface is on track to being REMOVED (or refactored
 * to the point of being near-unrecognizable).
 *
 * Please DO NOT #include this file in comm-central code, in your XULRunner
 * app or binary extensions.
 *
 * Please DO NOT #include this into new files even inside Gecko. It is more
 * likely than not that #including this header is the wrong thing to do.
 */
class nsIParser : public nsParserBase {
  public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPARSER_IID)

    /**
     * Select given content sink into parser for parser output
     * @update	gess5/11/98
     * @param   aSink is the new sink to be used by parser
     * @return  
     */
    NS_IMETHOD_(void) SetContentSink(nsIContentSink* aSink)=0;


    /**
     * retrieve the sink set into the parser 
     * @update	gess5/11/98
     * @return  current sink
     */
    NS_IMETHOD_(nsIContentSink*) GetContentSink(void)=0;

    /**
     *  Call this method once you've created a parser, and want to instruct it
	   *  about the command which caused the parser to be constructed. For example,
     *  this allows us to select a DTD which can do, say, view-source.
     *  
     *  @update  gess 3/25/98
     *  @param   aCommand -- ptrs to string that contains command
     *  @return	 nada
     */
    NS_IMETHOD_(void) GetCommand(nsCString& aCommand)=0;
    NS_IMETHOD_(void) SetCommand(const char* aCommand)=0;
    NS_IMETHOD_(void) SetCommand(eParserCommands aParserCommand)=0;

    /**
     *  Call this method once you've created a parser, and want to instruct it
     *  about what charset to load
     *  
     *  @update  ftang 4/23/99
     *  @param   aCharset- the charest of a document
     *  @param   aCharsetSource- the soure of the chares
     *  @return	 nada
     */
    NS_IMETHOD_(void) SetDocumentCharset(const nsACString& aCharset, int32_t aSource)=0;
    NS_IMETHOD_(void) GetDocumentCharset(nsACString& oCharset, int32_t& oSource)=0;

    /** 
     * Get the channel associated with this parser
     * @update harishd,gagan 07/17/01
     * @param aChannel out param that will contain the result
     * @return NS_OK if successful
     */
    NS_IMETHOD GetChannel(nsIChannel** aChannel) = 0;

    /** 
     * Get the DTD associated with this parser
     * @update vidur 9/29/99
     * @param aDTD out param that will contain the result
     * @return NS_OK if successful, NS_ERROR_FAILURE for runtime error
     */
    NS_IMETHOD GetDTD(nsIDTD** aDTD) = 0;
    
    /**
     * Get the nsIStreamListener for this parser
     */
    virtual nsIStreamListener* GetStreamListener() = 0;

    /**************************************************************************
     *  Parse methods always begin with an input source, and perform
     *  conversions until you wind up being emitted to the given contentsink
     *  (which may or may not be a proxy for the NGLayout content model).
     ************************************************************************/
    
    // Call this method to resume the parser from an unblocked state.
    // This can happen, for example, if parsing was interrupted and then the
    // consumer needed to restart the parser without waiting for more data.
    // This also happens after loading scripts, which unblock the parser in
    // order to process the output of document.write() and then need to
    // continue on with the page load on an enabled parser.
    NS_IMETHOD ContinueInterruptedParsing() = 0;
    
    // Stops parsing temporarily.
    NS_IMETHOD_(void) BlockParser() = 0;
    
    // Open up the parser for tokenization, building up content 
    // model..etc. However, this method does not resume parsing 
    // automatically. It's the callers' responsibility to restart
    // the parsing engine.
    NS_IMETHOD_(void) UnblockParser() = 0;

    /**
     * Asynchronously continues parsing.
     */
    NS_IMETHOD_(void) ContinueInterruptedParsingAsync() = 0;

    NS_IMETHOD_(bool) IsParserEnabled() = 0;
    NS_IMETHOD_(bool) IsComplete() = 0;
    
    NS_IMETHOD Parse(nsIURI* aURL,
                     nsIRequestObserver* aListener = nullptr,
                     void* aKey = 0,
                     nsDTDMode aMode = eDTDMode_autodetect) = 0;

    NS_IMETHOD Terminate(void) = 0;

    /**
     * This method gets called when you want to parse a fragment of HTML or XML
     * surrounded by the context |aTagStack|. It requires that the parser have
     * been given a fragment content sink.
     *
     * @param aSourceBuffer The XML or HTML that hasn't been parsed yet.
     * @param aTagStack The context of the source buffer.
     * @return Success or failure.
     */
    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             nsTArray<nsString>& aTagStack) = 0;

    /**
     * This method gets called when the tokens have been consumed, and it's time
     * to build the model via the content sink.
     * @update	gess5/11/98
     * @return  error code -- 0 if model building went well .
     */
    NS_IMETHOD BuildModel(void) = 0;

    /**
     *  Call this method to cancel any pending parsing events.
     *  Parsing events may be pending if all of the document's content
     *  has been passed to the parser but the parser has been interrupted
     *  because processing the tokens took too long.
     *  
     *  @update  kmcclusk 05/18/01
     *  @return  NS_OK if succeeded else ERROR.
     */

    NS_IMETHOD CancelParsingEvents() = 0;

    virtual void Reset() = 0;

    /**
     * True if the insertion point (per HTML5) is defined.
     */
    virtual bool IsInsertionPointDefined() = 0;

    /**
     * Call immediately before starting to evaluate a parser-inserted script.
     */
    virtual void BeginEvaluatingParserInsertedScript() = 0;

    /**
     * Call immediately after having evaluated a parser-inserted script.
     */
    virtual void EndEvaluatingParserInsertedScript() = 0;

    /**
     * Marks the HTML5 parser as not a script-created parser.
     */
    virtual void MarkAsNotScriptCreated(const char* aCommand) = 0;

    /**
     * True if this is a script-created HTML5 parser.
     */
    virtual bool IsScriptCreated() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIParser, NS_IPARSER_IID)

/* ===========================================================*
  Some useful constants...
 * ===========================================================*/

#include "prtypes.h"
#include "nsError.h"

const nsresult  kEOF              = NS_ERROR_HTMLPARSER_EOF;
const nsresult  kUnknownError     = NS_ERROR_HTMLPARSER_UNKNOWN;
const nsresult  kCantPropagate    = NS_ERROR_HTMLPARSER_CANTPROPAGATE;
const nsresult  kContextMismatch  = NS_ERROR_HTMLPARSER_CONTEXTMISMATCH;
const nsresult  kBadFilename      = NS_ERROR_HTMLPARSER_BADFILENAME;
const nsresult  kBadURL           = NS_ERROR_HTMLPARSER_BADURL;
const nsresult  kInvalidParserContext = NS_ERROR_HTMLPARSER_INVALIDPARSERCONTEXT;
const nsresult  kBlocked          = NS_ERROR_HTMLPARSER_BLOCK;
const nsresult  kBadStringLiteral = NS_ERROR_HTMLPARSER_UNTERMINATEDSTRINGLITERAL;
const nsresult  kHierarchyTooDeep = NS_ERROR_HTMLPARSER_HIERARCHYTOODEEP;
const nsresult  kFakeEndTag       = NS_ERROR_HTMLPARSER_FAKE_ENDTAG;
const nsresult  kNotAComment      = NS_ERROR_HTMLPARSER_INVALID_COMMENT;

#define NS_IPARSER_FLAG_UNKNOWN_MODE         0x00000000
#define NS_IPARSER_FLAG_QUIRKS_MODE          0x00000002
#define NS_IPARSER_FLAG_STRICT_MODE          0x00000004
#define NS_IPARSER_FLAG_AUTO_DETECT_MODE     0x00000010
#define NS_IPARSER_FLAG_VIEW_NORMAL          0x00000020
#define NS_IPARSER_FLAG_VIEW_SOURCE          0x00000040
#define NS_IPARSER_FLAG_VIEW_ERRORS          0x00000080
#define NS_IPARSER_FLAG_PLAIN_TEXT           0x00000100
#define NS_IPARSER_FLAG_XML                  0x00000200
#define NS_IPARSER_FLAG_HTML                 0x00000400
#define NS_IPARSER_FLAG_SCRIPT_ENABLED       0x00000800
#define NS_IPARSER_FLAG_FRAMES_ENABLED       0x00001000

#endif 
