/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sw=2 et tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsIMEStateManager.h"
#include "nsCOMPtr.h"
#include "nsIViewManager.h"
#include "nsIPresShell.h"
#include "nsISupports.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIEditorDocShell.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIDOMWindow.h"
#include "nsIDOMMouseEvent.h"
#include "nsContentUtils.h"
#include "nsINode.h"
#include "nsIFrame.h"
#include "nsRange.h"
#include "nsIDOMRange.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionListener.h"
#include "nsISelectionController.h"
#include "nsIMutationObserver.h"
#include "nsContentEventHandler.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsHTMLFormElement.h"
#include "mozilla/Attributes.h"
#include "nsEventDispatcher.h"
#include "TextComposition.h"

using namespace mozilla;
using namespace mozilla::widget;

/******************************************************************/
/* nsIMEStateManager                                              */
/******************************************************************/

nsIContent*    nsIMEStateManager::sContent      = nullptr;
nsPresContext* nsIMEStateManager::sPresContext  = nullptr;
bool           nsIMEStateManager::sInstalledMenuKeyboardListener = false;
bool           nsIMEStateManager::sInSecureInputMode = false;

nsTextStateManager* nsIMEStateManager::sTextStateObserver = nullptr;
TextCompositionArray* nsIMEStateManager::sTextCompositions = nullptr;

void
nsIMEStateManager::Shutdown()
{
  MOZ_ASSERT(!sTextCompositions || !sTextCompositions->Length());
  delete sTextCompositions;
  sTextCompositions = nullptr;
}

nsresult
nsIMEStateManager::OnDestroyPresContext(nsPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  // First, if there is a composition in the aPresContext, clean up it.
  if (sTextCompositions) {
    TextCompositionArray::index_type i =
      sTextCompositions->IndexOf(aPresContext);
    if (i != TextCompositionArray::NoIndex) {
      // there should be only one composition per presContext object.
      sTextCompositions->RemoveElementAt(i);
      MOZ_ASSERT(sTextCompositions->IndexOf(aPresContext) ==
                   TextCompositionArray::NoIndex);
    }
  }

  if (aPresContext != sPresContext)
    return NS_OK;
  nsCOMPtr<nsIWidget> widget = sPresContext->GetNearestWidget();
  if (widget) {
    IMEState newState = GetNewIMEState(sPresContext, nullptr);
    InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                              InputContextAction::LOST_FOCUS);
    SetIMEState(newState, nullptr, widget, action);
  }
  NS_IF_RELEASE(sContent);
  sPresContext = nullptr;
  OnTextStateBlur(nullptr, nullptr);
  return NS_OK;
}

nsresult
nsIMEStateManager::OnRemoveContent(nsPresContext* aPresContext,
                                   nsIContent* aContent)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  // First, if there is a composition in the aContent, clean up it.
  if (sTextCompositions) {
    TextComposition* compositionInContent =
      sTextCompositions->GetCompositionInContent(aPresContext, aContent);

    if (compositionInContent) {
      // Store the composition before accessing the native IME.
      TextComposition storedComposition = *compositionInContent;
      // Try resetting the native IME state.  Be aware, typically, this method
      // is called during the content being removed.  Then, the native
      // composition events which are caused by following APIs are ignored due
      // to unsafe to run script (in PresShell::HandleEvent()).
      nsCOMPtr<nsIWidget> widget = aPresContext->GetNearestWidget();
      if (widget) {
        nsresult rv =
          storedComposition.NotifyIME(REQUEST_TO_CANCEL_COMPOSITION);
        if (NS_FAILED(rv)) {
          storedComposition.NotifyIME(REQUEST_TO_COMMIT_COMPOSITION);
        }
        // By calling the APIs, the composition may have been finished normally.
        compositionInContent =
          sTextCompositions->GetCompositionFor(
                               storedComposition.GetPresContext(),
                               storedComposition.GetEventTargetNode());
      }
    }

    // If the compositionInContent is still available, we should finish the
    // composition just on the content forcibly.
    if (compositionInContent) {
      compositionInContent->SynthesizeCommit(true);
    }
  }

  if (!sPresContext || !sContent ||
      !nsContentUtils::ContentIsDescendantOf(sContent, aContent)) {
    return NS_OK;
  }

  // Current IME transaction should commit
  nsCOMPtr<nsIWidget> widget = sPresContext->GetNearestWidget();
  if (widget) {
    IMEState newState = GetNewIMEState(sPresContext, nullptr);
    InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                              InputContextAction::LOST_FOCUS);
    SetIMEState(newState, nullptr, widget, action);
  }

  NS_IF_RELEASE(sContent);
  sPresContext = nullptr;

  return NS_OK;
}

nsresult
nsIMEStateManager::OnChangeFocus(nsPresContext* aPresContext,
                                 nsIContent* aContent,
                                 InputContextAction::Cause aCause)
{
  InputContextAction action(aCause);
  return OnChangeFocusInternal(aPresContext, aContent, action);
}

nsresult
nsIMEStateManager::OnChangeFocusInternal(nsPresContext* aPresContext,
                                         nsIContent* aContent,
                                         InputContextAction aAction)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  nsCOMPtr<nsIWidget> widget = aPresContext->GetNearestWidget();
  if (!widget) {
    return NS_OK;
  }

  // Handle secure input mode for password field input.
  bool contentIsPassword = false;
  if (aContent && aContent->GetNameSpaceID() == kNameSpaceID_XHTML) {
    if (aContent->Tag() == nsGkAtoms::input) {
      nsAutoString type;
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, type);
      contentIsPassword = type.LowerCaseEqualsLiteral("password");
    }
  }
  if (sInSecureInputMode) {
    if (!contentIsPassword) {
      if (NS_SUCCEEDED(widget->EndSecureKeyboardInput())) {
        sInSecureInputMode = false;
      }
    }
  } else {
    if (contentIsPassword) {
      if (NS_SUCCEEDED(widget->BeginSecureKeyboardInput())) {
        sInSecureInputMode = true;
      }
    }
  }

  IMEState newState = GetNewIMEState(aPresContext, aContent);
  if (aPresContext == sPresContext && aContent == sContent) {
    // actual focus isn't changing, but if IME enabled state is changing,
    // we should do it.
    InputContext context = widget->GetInputContext();
    if (context.mIMEState.mEnabled == newState.mEnabled) {
      // the enabled state isn't changing.
      return NS_OK;
    }
    aAction.mFocusChange = InputContextAction::FOCUS_NOT_CHANGED;
  } else if (aAction.mFocusChange == InputContextAction::FOCUS_NOT_CHANGED) {
    // If aContent isn't null or aContent is null but editable, somebody gets
    // focus.
    bool gotFocus = aContent || (newState.mEnabled == IMEState::ENABLED);
    aAction.mFocusChange =
      gotFocus ? InputContextAction::GOT_FOCUS : InputContextAction::LOST_FOCUS;
  }

  // Current IME transaction should commit
  if (sPresContext) {
    nsCOMPtr<nsIWidget> oldWidget;
    if (sPresContext == aPresContext)
      oldWidget = widget;
    else
      oldWidget = sPresContext->GetNearestWidget();
    if (oldWidget) {
      NotifyIME(REQUEST_TO_COMMIT_COMPOSITION, oldWidget);
    }
  }

  // Update IME state for new focus widget
  SetIMEState(newState, aContent, widget, aAction);

  sPresContext = aPresContext;
  if (sContent != aContent) {
    NS_IF_RELEASE(sContent);
    NS_IF_ADDREF(sContent = aContent);
  }

  return NS_OK;
}

void
nsIMEStateManager::OnInstalledMenuKeyboardListener(bool aInstalling)
{
  sInstalledMenuKeyboardListener = aInstalling;

  InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
    aInstalling ? InputContextAction::MENU_GOT_PSEUDO_FOCUS :
                  InputContextAction::MENU_LOST_PSEUDO_FOCUS);
  OnChangeFocusInternal(sPresContext, sContent, action);
}

void
nsIMEStateManager::OnClickInEditor(nsPresContext* aPresContext,
                                   nsIContent* aContent,
                                   nsIDOMMouseEvent* aMouseEvent)
{
  if (sPresContext != aPresContext || sContent != aContent) {
    return;
  }

  nsCOMPtr<nsIWidget> widget = aPresContext->GetNearestWidget();
  NS_ENSURE_TRUE_VOID(widget);

  bool isTrusted;
  nsresult rv = aMouseEvent->GetIsTrusted(&isTrusted);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (!isTrusted) {
    return; // ignore untrusted event.
  }

  uint16_t button;
  rv = aMouseEvent->GetButton(&button);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (button != 0) {
    return; // not a left click event.
  }

  int32_t clickCount;
  rv = aMouseEvent->GetDetail(&clickCount);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (clickCount != 1) {
    return; // should notify only first click event.
  }

  InputContextAction action(InputContextAction::CAUSE_MOUSE,
                            InputContextAction::FOCUS_NOT_CHANGED);
  IMEState newState = GetNewIMEState(aPresContext, aContent);
  SetIMEState(newState, aContent, widget, action);
}

void
nsIMEStateManager::UpdateIMEState(const IMEState &aNewIMEState,
                                  nsIContent* aContent)
{
  if (!sPresContext) {
    NS_WARNING("ISM doesn't know which editor has focus");
    return;
  }
  nsCOMPtr<nsIWidget> widget = sPresContext->GetNearestWidget();
  if (!widget) {
    NS_WARNING("focused widget is not found");
    return;
  }

  // Don't update IME state when enabled state isn't actually changed.
  InputContext context = widget->GetInputContext();
  if (context.mIMEState.mEnabled == aNewIMEState.mEnabled) {
    return;
  }

  // commit current composition
  NotifyIME(REQUEST_TO_COMMIT_COMPOSITION, widget);

  InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                            InputContextAction::FOCUS_NOT_CHANGED);
  SetIMEState(aNewIMEState, aContent, widget, action);
}

IMEState
nsIMEStateManager::GetNewIMEState(nsPresContext* aPresContext,
                                  nsIContent*    aContent)
{
  // On Printing or Print Preview, we don't need IME.
  if (aPresContext->Type() == nsPresContext::eContext_PrintPreview ||
      aPresContext->Type() == nsPresContext::eContext_Print) {
    return IMEState(IMEState::DISABLED);
  }

  if (sInstalledMenuKeyboardListener) {
    return IMEState(IMEState::DISABLED);
  }

  if (!aContent) {
    // Even if there are no focused content, the focused document might be
    // editable, such case is design mode.
    nsIDocument* doc = aPresContext->Document();
    if (doc && doc->HasFlag(NODE_IS_EDITABLE)) {
      return IMEState(IMEState::ENABLED);
    }
    return IMEState(IMEState::DISABLED);
  }

  return aContent->GetDesiredIMEState();
}

// Helper class, used for IME enabled state change notification
class IMEEnabledStateChangedEvent : public nsRunnable {
public:
  IMEEnabledStateChangedEvent(uint32_t aState)
    : mState(aState)
  {
  }

  NS_IMETHOD Run() {
    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (observerService) {
      nsAutoString state;
      state.AppendInt(mState);
      observerService->NotifyObservers(nullptr, "ime-enabled-state-changed", state.get());
    }
    return NS_OK;
  }

private:
  uint32_t mState;
};

void
nsIMEStateManager::SetIMEState(const IMEState &aState,
                               nsIContent* aContent,
                               nsIWidget* aWidget,
                               InputContextAction aAction)
{
  NS_ENSURE_TRUE_VOID(aWidget);

  InputContext oldContext = aWidget->GetInputContext();

  InputContext context;
  context.mIMEState = aState;

  if (aContent && aContent->GetNameSpaceID() == kNameSpaceID_XHTML &&
      (aContent->Tag() == nsGkAtoms::input ||
       aContent->Tag() == nsGkAtoms::textarea)) {
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type,
                      context.mHTMLInputType);
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::inputmode,
                      context.mHTMLInputInputmode);
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::moz_action_hint,
                      context.mActionHint);

    // if we don't have an action hint and  return won't submit the form use "next"
    if (context.mActionHint.IsEmpty() && aContent->Tag() == nsGkAtoms::input) {
      bool willSubmit = false;
      nsCOMPtr<nsIFormControl> control(do_QueryInterface(aContent));
      mozilla::dom::Element* formElement = control->GetFormElement();
      nsCOMPtr<nsIForm> form;
      if (control) {
        // is this a form and does it have a default submit element?
        if ((form = do_QueryInterface(formElement)) && form->GetDefaultSubmitElement()) {
          willSubmit = true;
        // is this an html form and does it only have a single text input element?
        } else if (formElement && formElement->Tag() == nsGkAtoms::form && formElement->IsHTML() &&
                   static_cast<nsHTMLFormElement*>(formElement)->HasSingleTextControl()) {
          willSubmit = true;
        }
      }
      context.mActionHint.Assign(willSubmit ? control->GetType() == NS_FORM_INPUT_SEARCH
                                                ? NS_LITERAL_STRING("search")
                                                : NS_LITERAL_STRING("go")
                                            : formElement
                                                ? NS_LITERAL_STRING("next")
                                                : EmptyString());
    }
  }

  // XXX I think that we should use nsContentUtils::IsCallerChrome() instead
  //     of the process type.
  if (aAction.mCause == InputContextAction::CAUSE_UNKNOWN &&
      XRE_GetProcessType() != GeckoProcessType_Content) {
    aAction.mCause = InputContextAction::CAUSE_UNKNOWN_CHROME;
  }

  aWidget->SetInputContext(context, aAction);
  if (oldContext.mIMEState.mEnabled != context.mIMEState.mEnabled) {
    nsContentUtils::AddScriptRunner(
      new IMEEnabledStateChangedEvent(context.mIMEState.mEnabled));
  }
}

void
nsIMEStateManager::EnsureTextCompositionArray()
{
  if (sTextCompositions) {
    return;
  }
  sTextCompositions = new TextCompositionArray();
}

void
nsIMEStateManager::DispatchCompositionEvent(nsINode* aEventTargetNode,
                                            nsPresContext* aPresContext,
                                            nsEvent* aEvent,
                                            nsEventStatus* aStatus,
                                            nsDispatchingCallback* aCallBack)
{
  MOZ_ASSERT(aEvent->eventStructType == NS_COMPOSITION_EVENT ||
             aEvent->eventStructType == NS_TEXT_EVENT);
  if (!NS_IS_TRUSTED_EVENT(aEvent) ||
      (aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) != 0) {
    return;
  }

  EnsureTextCompositionArray();

  nsGUIEvent* GUIEvent = static_cast<nsGUIEvent*>(aEvent);

  TextComposition* composition =
    sTextCompositions->GetCompositionFor(GUIEvent->widget);
  if (!composition) {
    MOZ_ASSERT(GUIEvent->message == NS_COMPOSITION_START);
    TextComposition newComposition(aPresContext, aEventTargetNode, GUIEvent);
    composition = sTextCompositions->AppendElement(newComposition);
  }
#ifdef DEBUG
  else {
    MOZ_ASSERT(GUIEvent->message != NS_COMPOSITION_START);
  }
#endif // #ifdef DEBUG

  // Dispatch the event on composing target.
  composition->DispatchEvent(GUIEvent, aStatus, aCallBack);

  // WARNING: the |composition| might have been destroyed already.

  // Remove the ended composition from the array.
  if (aEvent->message == NS_COMPOSITION_END) {
    TextCompositionArray::index_type i =
      sTextCompositions->IndexOf(GUIEvent->widget);
    if (i != TextCompositionArray::NoIndex) {
      sTextCompositions->RemoveElementAt(i);
    }
  }
}

// static
nsresult
nsIMEStateManager::NotifyIME(NotificationToIME aNotification,
                             nsIWidget* aWidget)
{
  NS_ENSURE_TRUE(aWidget, NS_ERROR_INVALID_ARG);

  TextComposition* composition = nullptr;
  if (sTextCompositions) {
    composition = sTextCompositions->GetCompositionFor(aWidget);
  }
  if (!composition || !composition->IsSynthesizedForTests()) {
    switch (aNotification) {
      case NOTIFY_IME_OF_CURSOR_POS_CHANGED:
        return aWidget->ResetInputState();
      case REQUEST_TO_COMMIT_COMPOSITION:
        return composition ? aWidget->ResetInputState() : NS_OK;
      case REQUEST_TO_CANCEL_COMPOSITION:
        return composition ? aWidget->CancelIMEComposition() : NS_OK;
      default:
        MOZ_NOT_REACHED("Unsupported notification");
        return NS_ERROR_INVALID_ARG;
    }
    MOZ_NOT_REACHED(
      "Failed to handle the notification for non-synthesized composition");
  }

  // If the composition is synthesized events for automated tests, we should
  // dispatch composition events for emulating the native composition behavior.
  // NOTE: The dispatched events are discarded if it's not safe to run script.
  switch (aNotification) {
    case REQUEST_TO_COMMIT_COMPOSITION: {
      nsCOMPtr<nsIWidget> widget(aWidget);
      TextComposition backup = *composition;

      nsEventStatus status = nsEventStatus_eIgnore;
      if (!backup.GetLastData().IsEmpty()) {
        nsTextEvent textEvent(true, NS_TEXT_TEXT, widget);
        textEvent.theText = backup.GetLastData();
        textEvent.flags |= NS_EVENT_FLAG_SYNTHETIC_TEST_EVENT;
        widget->DispatchEvent(&textEvent, status);
        if (widget->Destroyed()) {
          return NS_OK;
        }
      }

      status = nsEventStatus_eIgnore;
      nsCompositionEvent endEvent(true, NS_COMPOSITION_END, widget);
      endEvent.data = backup.GetLastData();
      endEvent.flags |= NS_EVENT_FLAG_SYNTHETIC_TEST_EVENT;
      widget->DispatchEvent(&endEvent, status);

      return NS_OK;
    }
    case REQUEST_TO_CANCEL_COMPOSITION: {
      nsCOMPtr<nsIWidget> widget(aWidget);
      TextComposition backup = *composition;

      nsEventStatus status = nsEventStatus_eIgnore;
      if (!backup.GetLastData().IsEmpty()) {
        nsCompositionEvent updateEvent(true, NS_COMPOSITION_UPDATE, widget);
        updateEvent.data = backup.GetLastData();
        updateEvent.flags |= NS_EVENT_FLAG_SYNTHETIC_TEST_EVENT;
        widget->DispatchEvent(&updateEvent, status);
        if (widget->Destroyed()) {
          return NS_OK;
        }

        status = nsEventStatus_eIgnore;
        nsTextEvent textEvent(true, NS_TEXT_TEXT, widget);
        textEvent.theText = backup.GetLastData();
        textEvent.flags |= NS_EVENT_FLAG_SYNTHETIC_TEST_EVENT;
        widget->DispatchEvent(&textEvent, status);
        if (widget->Destroyed()) {
          return NS_OK;
        }
      }

      status = nsEventStatus_eIgnore;
      nsCompositionEvent endEvent(true, NS_COMPOSITION_END, widget);
      endEvent.data = backup.GetLastData();
      endEvent.flags |= NS_EVENT_FLAG_SYNTHETIC_TEST_EVENT;
      widget->DispatchEvent(&endEvent, status);

      return NS_OK;
    }
    default:
      return NS_OK;
  }
}

// static
nsresult
nsIMEStateManager::NotifyIME(NotificationToIME aNotification,
                             nsPresContext* aPresContext)
{
  NS_ENSURE_TRUE(aPresContext, NS_ERROR_INVALID_ARG);

  nsIWidget* widget = aPresContext->GetNearestWidget();
  if (!widget) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  return NotifyIME(aNotification, widget);
}


// nsTextStateManager notifies widget of any text and selection changes
//  in the currently focused editor
// sTextStateObserver points to the currently active nsTextStateManager
// sTextStateObserver is null if there is no focused editor

class nsTextStateManager MOZ_FINAL : public nsISelectionListener,
                                     public nsStubMutationObserver
{
public:
  nsTextStateManager();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISELECTIONLISTENER
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  nsresult Init(nsIWidget* aWidget,
                nsPresContext* aPresContext,
                nsINode* aNode,
                bool aWantUpdates);
  void     Destroy(void);

  nsCOMPtr<nsIWidget>            mWidget;
  nsCOMPtr<nsISelection>         mSel;
  nsCOMPtr<nsIContent>           mRootContent;
  nsCOMPtr<nsINode>              mEditableNode;
  bool                           mDestroying;

private:
  void NotifyContentAdded(nsINode* aContainer, int32_t aStart, int32_t aEnd);
};

nsTextStateManager::nsTextStateManager()
{
  mDestroying = false;
}

nsresult
nsTextStateManager::Init(nsIWidget* aWidget,
                         nsPresContext* aPresContext,
                         nsINode* aNode,
                         bool aWantUpdates)
{
  mWidget = aWidget;
  MOZ_ASSERT(mWidget);
  if (!aWantUpdates) {
    mEditableNode = aNode;
    return NS_OK;
  }

  nsIPresShell* presShell = aPresContext->PresShell();

  // get selection and root content
  nsCOMPtr<nsISelectionController> selCon;
  if (aNode->IsNodeOfType(nsINode::eCONTENT)) {
    nsIFrame* frame = static_cast<nsIContent*>(aNode)->GetPrimaryFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_UNEXPECTED);

    frame->GetSelectionController(aPresContext,
                                  getter_AddRefs(selCon));
  } else {
    // aNode is a document
    selCon = do_QueryInterface(presShell);
  }
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> sel;
  nsresult rv = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                     getter_AddRefs(sel));
  NS_ENSURE_TRUE(sel, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMRange> selDomRange;
  rv = sel->GetRangeAt(0, getter_AddRefs(selDomRange));

  if (NS_SUCCEEDED(rv)) {
    nsRange* selRange = static_cast<nsRange*>(selDomRange.get());
    NS_ENSURE_TRUE(selRange && selRange->GetStartParent(),
                   NS_ERROR_UNEXPECTED);

    mRootContent = selRange->GetStartParent()->
                     GetSelectionRootContent(presShell);
  } else {
    mRootContent = aNode->GetSelectionRootContent(presShell);
  }
  if (!mRootContent && aNode->IsNodeOfType(nsINode::eDOCUMENT)) {
    // The document node is editable, but there are no contents, this document
    // is not editable.
    return NS_ERROR_NOT_AVAILABLE;
  }
  NS_ENSURE_TRUE(mRootContent, NS_ERROR_UNEXPECTED);

  // add text change observer
  mRootContent->AddMutationObserver(this);

  // add selection change listener
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(sel));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_UNEXPECTED);
  rv = selPrivate->AddSelectionListener(this);
  NS_ENSURE_SUCCESS(rv, rv);
  mSel = sel;

  mEditableNode = aNode;
  return NS_OK;
}

void
nsTextStateManager::Destroy(void)
{
  if (mSel) {
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(mSel));
    if (selPrivate)
      selPrivate->RemoveSelectionListener(this);
    mSel = nullptr;
  }
  if (mRootContent) {
    mRootContent->RemoveMutationObserver(this);
    mRootContent = nullptr;
  }
  mEditableNode = nullptr;
  mWidget = nullptr;
}

NS_IMPL_ISUPPORTS2(nsTextStateManager,
                   nsIMutationObserver,
                   nsISelectionListener)

// Helper class, used for selection change notification
class SelectionChangeEvent : public nsRunnable {
public:
  SelectionChangeEvent(nsIWidget *widget)
    : mWidget(widget)
  {
    MOZ_ASSERT(mWidget);
  }

  NS_IMETHOD Run() {
    if(mWidget) {
        mWidget->OnIMESelectionChange();
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIWidget> mWidget;
};

nsresult
nsTextStateManager::NotifySelectionChanged(nsIDOMDocument* aDoc,
                                           nsISelection* aSel,
                                           int16_t aReason)
{
  int32_t count = 0;
  nsresult rv = aSel->GetRangeCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count > 0 && mWidget) {
    nsContentUtils::AddScriptRunner(new SelectionChangeEvent(mWidget));
  }
  return NS_OK;
}

// Helper class, used for text change notification
class TextChangeEvent : public nsRunnable {
public:
  TextChangeEvent(nsIWidget *widget,
                  uint32_t start, uint32_t oldEnd, uint32_t newEnd)
    : mWidget(widget)
    , mStart(start)
    , mOldEnd(oldEnd)
    , mNewEnd(newEnd)
  {
    MOZ_ASSERT(mWidget);
  }

  NS_IMETHOD Run() {
    if(mWidget) {
        mWidget->OnIMETextChange(mStart, mOldEnd, mNewEnd);
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIWidget> mWidget;
  uint32_t mStart, mOldEnd, mNewEnd;
};

void
nsTextStateManager::CharacterDataChanged(nsIDocument* aDocument,
                                         nsIContent* aContent,
                                         CharacterDataChangeInfo* aInfo)
{
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
               "character data changed for non-text node");

  uint32_t offset = 0;
  // get offsets of change and fire notification
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, aContent, aInfo->mChangeStart, &offset)))
    return;

  uint32_t oldEnd = offset + aInfo->mChangeEnd - aInfo->mChangeStart;
  uint32_t newEnd = offset + aInfo->mReplaceLength;

  nsContentUtils::AddScriptRunner(
      new TextChangeEvent(mWidget, offset, oldEnd, newEnd));
}

void
nsTextStateManager::NotifyContentAdded(nsINode* aContainer,
                                       int32_t aStartIndex,
                                       int32_t aEndIndex)
{
  uint32_t offset = 0, newOffset = 0;
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, aContainer, aStartIndex, &offset)))
    return;

  // get offset at the end of the last added node
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    aContainer->GetChildAt(aStartIndex),
                    aContainer, aEndIndex, &newOffset)))
    return;

  // fire notification
  if (newOffset)
    nsContentUtils::AddScriptRunner(
        new TextChangeEvent(mWidget, offset, offset, offset + newOffset));
}

void
nsTextStateManager::ContentAppended(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aFirstNewContent,
                                    int32_t aNewIndexInContainer)
{
  NotifyContentAdded(aContainer, aNewIndexInContainer,
                     aContainer->GetChildCount());
}

void
nsTextStateManager::ContentInserted(nsIDocument* aDocument,
                                     nsIContent* aContainer,
                                     nsIContent* aChild,
                                     int32_t aIndexInContainer)
{
  NotifyContentAdded(NODE_FROM(aContainer, aDocument),
                     aIndexInContainer, aIndexInContainer + 1);
}

void
nsTextStateManager::ContentRemoved(nsIDocument* aDocument,
                                   nsIContent* aContainer,
                                   nsIContent* aChild,
                                   int32_t aIndexInContainer,
                                   nsIContent* aPreviousSibling)
{
  uint32_t offset = 0, childOffset = 1;
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, NODE_FROM(aContainer, aDocument),
                    aIndexInContainer, &offset)))
    return;

  // get offset at the end of the deleted node
  if (aChild->IsNodeOfType(nsINode::eTEXT))
    childOffset = aChild->TextLength();
  else if (0 < aChild->GetChildCount())
    childOffset = aChild->GetChildCount();

  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    aChild, aChild, childOffset, &childOffset)))
    return;

  // fire notification
  if (childOffset)
    nsContentUtils::AddScriptRunner(
        new TextChangeEvent(mWidget, offset, offset + childOffset, offset));
}

static bool IsEditable(nsINode* node) {
  if (node->IsEditable()) {
    return true;
  }
  // |node| might be readwrite (for example, a text control)
  if (node->IsElement() && node->AsElement()->State().HasState(NS_EVENT_STATE_MOZ_READWRITE)) {
    return true;
  }
  return false;
}

static nsINode* GetRootEditableNode(nsPresContext* aPresContext,
                                    nsIContent* aContent)
{
  if (aContent) {
    nsINode* root = nullptr;
    nsINode* node = aContent;
    while (node && IsEditable(node)) {
      root = node;
      node = node->GetParentNode();
    }
    return root;
  }
  if (aPresContext) {
    nsIDocument* document = aPresContext->Document();
    if (document && document->IsEditable())
      return document;
  }
  return nullptr;
}

nsresult
nsIMEStateManager::OnTextStateBlur(nsPresContext* aPresContext,
                                   nsIContent* aContent)
{
  if (!sTextStateObserver || sTextStateObserver->mDestroying ||
      sTextStateObserver->mEditableNode ==
          GetRootEditableNode(aPresContext, aContent))
    return NS_OK;

  sTextStateObserver->mDestroying = true;
  sTextStateObserver->mWidget->OnIMEFocusChange(false);
  sTextStateObserver->Destroy();
  NS_RELEASE(sTextStateObserver);
  return NS_OK;
}

nsresult
nsIMEStateManager::OnTextStateFocus(nsPresContext* aPresContext,
                                    nsIContent* aContent)
{
  if (sTextStateObserver) return NS_OK;

  nsINode *editableNode = GetRootEditableNode(aPresContext, aContent);
  if (!editableNode) return NS_OK;

  nsIPresShell* shell = aPresContext->GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_NOT_AVAILABLE);
  
  nsIViewManager* vm = shell->GetViewManager();
  NS_ENSURE_TRUE(vm, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIWidget> widget;
  nsresult rv = vm->GetRootWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);
  if (!widget) {
    return NS_OK; // Sometimes, there are no widgets.
  }

  rv = widget->OnIMEFocusChange(true);
  if (rv == NS_ERROR_NOT_IMPLEMENTED)
    return NS_OK;
  NS_ENSURE_SUCCESS(rv, rv);

  bool wantUpdates = rv != NS_SUCCESS_IME_NO_UPDATES;

  // OnIMEFocusChange may cause focus and sTextStateObserver to change
  // In that case return and keep the current sTextStateObserver
  NS_ENSURE_TRUE(!sTextStateObserver, NS_OK);

  sTextStateObserver = new nsTextStateManager();
  NS_ENSURE_TRUE(sTextStateObserver, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(sTextStateObserver);
  rv = sTextStateObserver->Init(widget, aPresContext,
                                editableNode, wantUpdates);
  if (NS_FAILED(rv)) {
    sTextStateObserver->mDestroying = true;
    sTextStateObserver->Destroy();
    NS_RELEASE(sTextStateObserver);
    widget->OnIMEFocusChange(false);
    return rv;
  }
  return NS_OK;
}

nsresult
nsIMEStateManager::GetFocusSelectionAndRoot(nsISelection** aSel,
                                            nsIContent** aRoot)
{
  if (!sTextStateObserver || !sTextStateObserver->mEditableNode ||
      !sTextStateObserver->mSel)
    return NS_ERROR_NOT_AVAILABLE;

  NS_ASSERTION(sTextStateObserver->mSel && sTextStateObserver->mRootContent,
               "uninitialized text state observer");
  NS_ADDREF(*aSel = sTextStateObserver->mSel);
  NS_ADDREF(*aRoot = sTextStateObserver->mRootContent);
  return NS_OK;
}
