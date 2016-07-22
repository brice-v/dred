// Copyright (C) 2016 David Reid. See included LICENSE file.

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <float.h>
#include <math.h>

#ifndef DRGUI_PRIVATE
#define DRGUI_PRIVATE static
#endif

/////////////////////////////////////////////////////////////////
//
// PRIVATE CORE API
//
/////////////////////////////////////////////////////////////////


// Context Flags
#define IS_CONTEXT_DEAD                     (1U << 0)
#define IS_AUTO_DIRTY_DISABLED              (1U << 1)
#define IS_RELEASING_KEYBOARD               (1U << 2)

// Element Flags
#define IS_ELEMENT_HIDDEN                   (1U << 0)
#define IS_ELEMENT_CLIPPING_DISABLED        (1U << 1)
#define IS_ELEMENT_DEAD                     (1U << 31)


static int drgui__strcpy_s(char* dst, size_t dstSizeInBytes, const char* src)
{
#ifdef _MSC_VER
    return strcpy_s(dst, dstSizeInBytes, src);
#else
    if (dst == 0) {
        return EINVAL;
    }
    if (dstSizeInBytes == 0) {
        return ERANGE;
    }
    if (src == 0) {
        dst[0] = '\0';
        return EINVAL;
    }

    size_t i;
    for (i = 0; i < dstSizeInBytes && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }

    if (i < dstSizeInBytes) {
        dst[i] = '\0';
        return 0;
    }

    dst[0] = '\0';
    return ERANGE;
#endif
}

int drgui__strncpy_s(char* dst, size_t dstSizeInBytes, const char* src, size_t count)
{
#ifdef _MSC_VER
    return strncpy_s(dst, dstSizeInBytes, src, count);
#else
    if (dst == 0) {
        return EINVAL;
    }
    if (dstSizeInBytes == 0) {
        return EINVAL;
    }
    if (src == 0) {
        dst[0] = '\0';
        return EINVAL;
    }

    size_t maxcount = count;
    if (count == ((size_t)-1) || count >= dstSizeInBytes) {        // -1 = _TRUNCATE
        maxcount = dstSizeInBytes - 1;
    }

    size_t i;
    for (i = 0; i < maxcount && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }

    if (src[i] == '\0' || i == count || count == ((size_t)-1)) {
        dst[i] = '\0';
        return 0;
    }

    dst[0] = '\0';
    return ERANGE;
#endif
}


/// Increments the inbound event counter
///
/// @remarks
///     This is called from every drgui_post_inbound_event_*() function and is used to keep track of whether or
///     not an inbound event is being processed. We need to track this because if we are in the middle of event
///     processing and an element is deleted, we want to delay it's deletion until the end of the event processing.
///     @par
///     Use drgui_end_inbound_event() to decrement the counter.
void drgui_begin_inbound_event(drgui_context* pContext);

/// Decrements the inbound event counter.
///
/// @remarks
///     This is called from every drgui_post_inbound_event_*() function.
///     @par
///     When the internal counter reaches zero, deleted elements will be garbage collected.
void drgui_end_inbound_event(drgui_context* pContext);

/// Determines whether or not inbound events are being processed.
///
/// @remarks
///     This is used to determine whether or not an element can be deleted immediately or should be garbage collected
///     at the end of event processing.
bool drgui_is_handling_inbound_event(const drgui_context* pContext);


/// Increments the outbound event counter.
///
/// @remarks
///     This will validate that the given element is allowed to have an event posted. When false is returned, nothing
///     will have been locked and the outbound event should be cancelled.
///     @par
///     This will return false if the given element has been marked as dead, or if there is some other reason it should
///     not be receiving events.
bool drgui_begin_outbound_event(drgui_element* pElement);

/// Decrements the outbound event counter.
void drgui_end_outbound_event(drgui_element* pElement);

/// Determines whether or not and outbound event is being processed.
bool drgui_is_handling_outbound_event(drgui_context* pContext);


/// Marks the given element as dead.
void drgui_mark_element_as_dead(drgui_element* pElement);

/// Determines whether or not the given element is marked as dead.
bool drgui_is_element_marked_as_dead(const drgui_element* pElement);

/// Deletes every element that has been marked as dead.
void drgui_delete_elements_marked_as_dead(drgui_context* pContext);


/// Marks the given context as deleted.
void drgui_mark_context_as_dead(drgui_context* pContext);

/// Determines whether or not the given context is marked as dead.
bool drgui_is_context_marked_as_dead(const drgui_context* pContext);


/// Deletes the given context for real.
///
/// If a context is deleted during the processing of an inbound event it will not be deleting immediately - this
/// will delete the context for real.
void drgui_delete_context_for_real(drgui_context* pContext);

/// Deletes the given element for real.
///
/// Sometimes an element will not be deleted straight away but instead just marked as dead. We use this to delete
/// the given element for real.
void drgui_delete_element_for_real(drgui_element* pElement);


/// Orphans the given element without triggering a redraw of the parent nor the child.
void drgui_detach_without_redraw(drgui_element* pChildElement);

/// Appends the given element without first detaching it from the old parent, nor does it post a redraw.
void drgui_append_without_detach_or_redraw(drgui_element* pChildElement, drgui_element* pParentElement);

/// Appends the given element without first detaching it from the old parent.
void drgui_append_without_detach(drgui_element* pChildElement, drgui_element* pParentElement);

/// Prepends the given element without first detaching it from the old parent, nor does it post a redraw.
void drgui_prepend_without_detach_or_redraw(drgui_element* pChildElement, drgui_element* pParentElement);

/// Prepends the given element without first detaching it from the old parent.
void drgui_prepend_without_detach(drgui_element* pChildElement, drgui_element* pParentElement);

/// Appends an element to another as it's sibling, but does not detach it from the previous parent nor trigger a redraw.
void drgui_append_sibling_without_detach_or_redraw(drgui_element* pElementToAppend, drgui_element* pElementToAppendTo);

/// Appends an element to another as it's sibling, but does not detach it from the previous parent.
void drgui_append_sibling_without_detach(drgui_element* pElementToAppend, drgui_element* pElementToAppendTo);

/// Prepends an element to another as it's sibling, but does not detach it from the previous parent nor trigger a redraw.
void drgui_prepend_sibling_without_detach_or_redraw(drgui_element* pElementToPrepend, drgui_element* pElementToPrependTo);

/// Prepends an element to another as it's sibling, but does not detach it from the previous parent.
void drgui_prepend_sibling_without_detach(drgui_element* pElementToPrepend, drgui_element* pElementToPrependTo);


/// Begins accumulating an invalidation rectangle.
void drgui_begin_auto_dirty(drgui_element* pElement);

/// Ends accumulating the invalidation rectangle and posts on_dirty is auto-dirty is enabled.
void drgui_end_auto_dirty(drgui_element* pElement);

/// Marks the given region of the given top level element as dirty, but only if automatic dirtying is enabled.
///
/// @remarks
///     This is equivalent to drgui_begin_auto_dirty() immediately followed by drgui_end_auto_dirty().
void drgui_auto_dirty(drgui_element* pTopLevelElement, drgui_rect rect);


/// Recursively applies the given offset to the absolute positions of the children of the given element.
///
/// @remarks
///     This is called when the absolute position of an element is changed.
void drgui_apply_offset_to_children_recursive(drgui_element* pParentElement, float offsetX, float offsetY);


/// The function to call when the mouse may have entered into a new element.
void drgui_update_mouse_enter_and_leave_state(drgui_context* pContext, drgui_element* pNewElementUnderMouse);


/// Functions for posting outbound events.
void drgui_post_outbound_event_move(drgui_element* pElement, float newRelativePosX, float newRelativePosY);
void drgui_post_outbound_event_size(drgui_element* pElement, float newWidth, float newHeight);
void drgui_post_outbound_event_mouse_enter(drgui_element* pElement);
void drgui_post_outbound_event_mouse_leave(drgui_element* pElement);
void drgui_post_outbound_event_mouse_move(drgui_element* pElement, int relativeMousePosX, int relativeMousePosY, int stateFlags);
void drgui_post_outbound_event_mouse_button_down(drgui_element* pElement, int mouseButton, int relativeMousePosX, int relativeMousePosY, int stateFlags);
void drgui_post_outbound_event_mouse_button_up(drgui_element* pElement, int mouseButton, int relativeMousePosX, int relativeMousePosY, int stateFlags);
void drgui_post_outbound_event_mouse_button_dblclick(drgui_element* pElement, int mouseButton, int relativeMousePosX, int relativeMousePosY, int stateFlags);
void drgui_post_outbound_event_mouse_wheel(drgui_element* pElement, int delta, int relativeMousePosX, int relativeMousePosY, int stateFlags);
void drgui_post_outbound_event_key_down(drgui_element* pElement, drgui_key key, int stateFlags);
void drgui_post_outbound_event_key_up(drgui_element* pElement, drgui_key key, int stateFlags);
void drgui_post_outbound_event_printable_key_down(drgui_element* pElement, unsigned int character, int stateFlags);
void drgui_post_outbound_event_dirty(drgui_element* pElement, drgui_rect relativeRect);
void drgui_post_outbound_event_dirty_global(drgui_element* pElement, drgui_rect relativeRect);
void drgui_post_outbound_event_capture_mouse(drgui_element* pElement);
void drgui_post_outbound_event_capture_mouse_global(drgui_element* pElement);
void drgui_post_outbound_event_release_mouse(drgui_element* pElement);
void drgui_post_outbound_event_release_mouse_global(drgui_element* pElement);
void drgui_post_outbound_event_capture_keyboard(drgui_element* pElement, drgui_element* pPrevCapturedElement);
void drgui_post_outbound_event_capture_keyboard_global(drgui_element* pElement, drgui_element* pPrevCapturedElement);
void drgui_post_outbound_event_release_keyboard(drgui_element* pElement, drgui_element* pNewCapturedElement);
void drgui_post_outbound_event_release_keyboard_global(drgui_element* pElement, drgui_element* pNewCapturedElement);

/// Posts a log message.
void drgui_log(drgui_context* pContext, const char* message);


void drgui_begin_inbound_event(drgui_context* pContext)
{
    assert(pContext != NULL);

    pContext->inboundEventCounter += 1;
}

void drgui_end_inbound_event(drgui_context* pContext)
{
    assert(pContext != NULL);
    assert(pContext->inboundEventCounter > 0);

    pContext->inboundEventCounter -= 1;


    // Here is where we want to clean up any elements that are marked as dead. When events are being handled elements are not deleted
    // immediately but instead only marked for deletion. This function will be called at the end of event processing which makes it
    // an appropriate place for cleaning up dead elements.
    if (!drgui_is_handling_inbound_event(pContext))
    {
        drgui_delete_elements_marked_as_dead(pContext);

        // If the context has been marked for deletion than we will need to delete that too.
        if (drgui_is_context_marked_as_dead(pContext))
        {
            drgui_delete_context_for_real(pContext);
        }
    }
}

bool drgui_is_handling_inbound_event(const drgui_context* pContext)
{
    assert(pContext != NULL);

    return pContext->inboundEventCounter > 0;
}



bool drgui_begin_outbound_event(drgui_element* pElement)
{
    assert(pElement != NULL);
    assert(pElement->pContext != NULL);


    // We want to cancel the outbound event if the element is marked as dead.
    if (drgui_is_element_marked_as_dead(pElement)) {
        drgui_log(pElement->pContext, "WARNING: Attemping to post an event to an element that is marked for deletion.");
        return false;
    }


    // At this point everything should be fine so we just increment the count (which should never go above 1) and return true.
    pElement->pContext->outboundEventLockCounter += 1;

    return true;
}

void drgui_end_outbound_event(drgui_element* pElement)
{
    assert(pElement != NULL);
    assert(pElement->pContext != NULL);
    assert(pElement->pContext->outboundEventLockCounter > 0);

    pElement->pContext->outboundEventLockCounter -= 1;
}

bool drgui_is_handling_outbound_event(drgui_context* pContext)
{
    assert(pContext != NULL);
    return pContext->outboundEventLockCounter > 0;
}


void drgui_mark_element_as_dead(drgui_element* pElement)
{
    assert(pElement != NULL);
    assert(pElement->pContext != NULL);

    pElement->flags |= IS_ELEMENT_DEAD;


    if (pElement->pContext->pFirstDeadElement != NULL) {
        pElement->pNextDeadElement = pElement->pContext->pFirstDeadElement;
    }

    pElement->pContext->pFirstDeadElement = pElement;
}

bool drgui_is_element_marked_as_dead(const drgui_element* pElement)
{
    if (pElement == NULL) {
        return false;
    }

    return (pElement->flags & IS_ELEMENT_DEAD) != 0;
}

void drgui_delete_elements_marked_as_dead(drgui_context* pContext)
{
    assert(pContext != NULL);

    while (pContext->pFirstDeadElement != NULL)
    {
        drgui_element* pDeadElement = pContext->pFirstDeadElement;
        pContext->pFirstDeadElement = pContext->pFirstDeadElement->pNextDeadElement;

        drgui_delete_element_for_real(pDeadElement);
    }
}


void drgui_mark_context_as_dead(drgui_context* pContext)
{
    assert(pContext != NULL);
    assert(!drgui_is_context_marked_as_dead(pContext));

    pContext->flags |= IS_CONTEXT_DEAD;
}

bool drgui_is_context_marked_as_dead(const drgui_context* pContext)
{
    assert(pContext != NULL);

    return (pContext->flags & IS_CONTEXT_DEAD) != 0;
}



void drgui_delete_context_for_real(drgui_context* pContext)
{
    assert(pContext != NULL);

    // All elements marked as dead need to be deleted.
    drgui_delete_elements_marked_as_dead(pContext);

    free(pContext);
}

void drgui_delete_element_for_real(drgui_element* pElementToDelete)
{
    assert(pElementToDelete != NULL);

    drgui_context* pContext = pElementToDelete->pContext;

    // If the element is marked as dead
    if (drgui_is_element_marked_as_dead(pElementToDelete)) {
        if (pContext->pFirstDeadElement == pElementToDelete) {
            pContext->pFirstDeadElement = pContext->pFirstDeadElement->pNextDeadElement;
        } else {
            drgui_element* pPrevDeadElement = pContext->pFirstDeadElement;
            while (pPrevDeadElement != NULL) {
                if (pPrevDeadElement->pNextDeadElement == pElementToDelete) {
                    break;
                }

                pPrevDeadElement = pPrevDeadElement->pNextDeadElement;
            }

            if (pPrevDeadElement != NULL) {
                pElementToDelete->pNextDeadElement = pElementToDelete->pNextDeadElement;
            }
        }
    }

    free(pElementToDelete);
}


void drgui_detach_without_redraw(drgui_element* pElement)
{
    if (pElement->pParent != NULL) {
        if (pElement->pParent->pFirstChild == pElement) {
            pElement->pParent->pFirstChild = pElement->pNextSibling;
        }

        if (pElement->pParent->pLastChild == pElement) {
            pElement->pParent->pLastChild = pElement->pPrevSibling;
        }


        if (pElement->pPrevSibling != NULL) {
            pElement->pPrevSibling->pNextSibling = pElement->pNextSibling;
        }

        if (pElement->pNextSibling != NULL) {
            pElement->pNextSibling->pPrevSibling = pElement->pPrevSibling;
        }
    }

    pElement->pParent      = NULL;
    pElement->pPrevSibling = NULL;
    pElement->pNextSibling = NULL;
}

void drgui_append_without_detach_or_redraw(drgui_element* pChildElement, drgui_element* pParentElement)
{
    pChildElement->pParent = pParentElement;
    if (pChildElement->pParent != NULL) {
        if (pChildElement->pParent->pLastChild != NULL) {
            pChildElement->pPrevSibling = pChildElement->pParent->pLastChild;
            pChildElement->pPrevSibling->pNextSibling = pChildElement;
        }

        if (pChildElement->pParent->pFirstChild == NULL) {
            pChildElement->pParent->pFirstChild = pChildElement;
        }

        pChildElement->pParent->pLastChild = pChildElement;
    }
}

void drgui_append_without_detach(drgui_element* pChildElement, drgui_element* pParentElement)
{
    drgui_append_without_detach_or_redraw(pChildElement, pParentElement);
    drgui_auto_dirty(pChildElement, drgui_make_rect(0, 0, pChildElement->width, pChildElement->height));
}

void drgui_prepend_without_detach_or_redraw(drgui_element* pChildElement, drgui_element* pParentElement)
{
    pChildElement->pParent = pParentElement;
    if (pChildElement->pParent != NULL) {
        if (pChildElement->pParent->pFirstChild != NULL) {
            pChildElement->pNextSibling = pChildElement->pParent->pFirstChild;
            pChildElement->pNextSibling->pPrevSibling = pChildElement;
        }

        if (pChildElement->pParent->pLastChild == NULL) {
            pChildElement->pParent->pLastChild = pChildElement;
        }

        pChildElement->pParent->pFirstChild = pChildElement;
    }
}

void drgui_prepend_without_detach(drgui_element* pChildElement, drgui_element* pParentElement)
{
    drgui_prepend_without_detach_or_redraw(pChildElement, pParentElement);
    drgui_auto_dirty(pChildElement, drgui_make_rect(0, 0, pChildElement->width, pChildElement->height));
}

void drgui_append_sibling_without_detach_or_redraw(drgui_element* pElementToAppend, drgui_element* pElementToAppendTo)
{
    assert(pElementToAppend   != NULL);
    assert(pElementToAppendTo != NULL);

    pElementToAppend->pParent = pElementToAppendTo->pParent;
    if (pElementToAppend->pParent != NULL)
    {
        pElementToAppend->pNextSibling = pElementToAppendTo->pNextSibling;
        pElementToAppend->pPrevSibling = pElementToAppendTo;

        pElementToAppendTo->pNextSibling->pPrevSibling = pElementToAppend;
        pElementToAppendTo->pNextSibling = pElementToAppend;

        if (pElementToAppend->pParent->pLastChild == pElementToAppendTo) {
            pElementToAppend->pParent->pLastChild = pElementToAppend;
        }
    }
}

void drgui_append_sibling_without_detach(drgui_element* pElementToAppend, drgui_element* pElementToAppendTo)
{
    drgui_append_sibling_without_detach_or_redraw(pElementToAppend, pElementToAppendTo);
    drgui_auto_dirty(pElementToAppend, drgui_make_rect(0, 0, pElementToAppend->width, pElementToAppend->height));
}

void drgui_prepend_sibling_without_detach_or_redraw(drgui_element* pElementToPrepend, drgui_element* pElementToPrependTo)
{
    assert(pElementToPrepend   != NULL);
    assert(pElementToPrependTo != NULL);

    pElementToPrepend->pParent = pElementToPrependTo->pParent;
    if (pElementToPrepend->pParent != NULL)
    {
        pElementToPrepend->pPrevSibling = pElementToPrependTo->pNextSibling;
        pElementToPrepend->pNextSibling = pElementToPrependTo;

        pElementToPrependTo->pPrevSibling->pNextSibling = pElementToPrepend;
        pElementToPrependTo->pNextSibling = pElementToPrepend;

        if (pElementToPrepend->pParent->pFirstChild == pElementToPrependTo) {
            pElementToPrepend->pParent->pFirstChild = pElementToPrepend;
        }
    }
}

void drgui_prepend_sibling_without_detach(drgui_element* pElementToPrepend, drgui_element* pElementToPrependTo)
{
    drgui_prepend_sibling_without_detach_or_redraw(pElementToPrepend, pElementToPrependTo);
    drgui_auto_dirty(pElementToPrepend, drgui_make_rect(0, 0, pElementToPrepend->width, pElementToPrepend->height));
}


void drgui_begin_auto_dirty(drgui_element* pElement)
{
    assert(pElement           != NULL);
    assert(pElement->pContext != NULL);

    if (drgui_is_auto_dirty_enabled(pElement->pContext)) {
        drgui_begin_dirty(pElement);
    }
}

void drgui_end_auto_dirty(drgui_element* pElement)
{
    assert(pElement != NULL);

    drgui_context* pContext = pElement->pContext;
    assert(pContext != NULL);

    if (drgui_is_auto_dirty_enabled(pContext)) {
        drgui_end_dirty(pElement);
    }
}

void drgui_auto_dirty(drgui_element* pElement, drgui_rect relativeRect)
{
    assert(pElement != NULL);
    assert(pElement->pContext != NULL);

    if (drgui_is_auto_dirty_enabled(pElement->pContext)) {
        drgui_dirty(pElement, relativeRect);
    }
}


void drgui__change_cursor(drgui_element* pElement, drgui_cursor_type cursor)
{
    if (pElement == NULL || pElement->pContext == NULL) {
        return;
    }

    pElement->pContext->currentCursor = cursor;

    if (pElement->pContext->onChangeCursor) {
        pElement->pContext->onChangeCursor(pElement, cursor);
    }
}



void drgui_apply_offset_to_children_recursive(drgui_element* pParentElement, float offsetX, float offsetY)
{
    assert(pParentElement != NULL);

    for (drgui_element* pChild = pParentElement->pFirstChild; pChild != NULL; pChild = pChild->pNextSibling)
    {
        drgui_begin_auto_dirty(pParentElement);
        {
            drgui_auto_dirty(pParentElement, drgui_get_local_rect(pParentElement));
            pChild->absolutePosX += offsetX;
            pChild->absolutePosY += offsetY;

            drgui_apply_offset_to_children_recursive(pChild, offsetX, offsetY);
        }
        drgui_end_auto_dirty(pParentElement);
    }
}

DRGUI_PRIVATE void drgui_post_on_mouse_leave_recursive(drgui_context* pContext, drgui_element* pNewElementUnderMouse, drgui_element* pOldElementUnderMouse)
{
    (void)pContext;

    drgui_element* pOldAncestor = pOldElementUnderMouse;
    while (pOldAncestor != NULL)
    {
        bool isOldElementUnderMouse = pNewElementUnderMouse == pOldAncestor || drgui_is_ancestor(pOldAncestor, pNewElementUnderMouse);
        if (!isOldElementUnderMouse)
        {
            drgui_post_outbound_event_mouse_leave(pOldAncestor);
        }

        pOldAncestor = pOldAncestor->pParent;
    }
}

DRGUI_PRIVATE void drgui_post_on_mouse_enter_recursive(drgui_context* pContext, drgui_element* pNewElementUnderMouse, drgui_element* pOldElementUnderMouse)
{
    if (pNewElementUnderMouse == NULL) {
        return;
    }


    if (pNewElementUnderMouse->pParent != NULL) {
        drgui_post_on_mouse_enter_recursive(pContext, pNewElementUnderMouse->pParent, pOldElementUnderMouse);
    }

    bool wasNewElementUnderMouse = pOldElementUnderMouse == pNewElementUnderMouse || drgui_is_ancestor(pNewElementUnderMouse, pOldElementUnderMouse);
    if (!wasNewElementUnderMouse)
    {
        drgui_post_outbound_event_mouse_enter(pNewElementUnderMouse);
    }
}

void drgui_update_mouse_enter_and_leave_state(drgui_context* pContext, drgui_element* pNewElementUnderMouse)
{
    if (pContext == NULL) {
        return;
    }

    drgui_element* pOldElementUnderMouse = pContext->pElementUnderMouse;
    if (pOldElementUnderMouse != pNewElementUnderMouse)
    {
        // We don't change the enter and leave state if an element is capturing the mouse.
        if (pContext->pElementWithMouseCapture == NULL)
        {
            pContext->pElementUnderMouse = pNewElementUnderMouse;

            drgui_cursor_type newCursor = drgui_cursor_default;
            if (pNewElementUnderMouse != NULL) {
                newCursor = pNewElementUnderMouse->cursor;
            }


            // It's intuitive to check that the new cursor is different to the old one before trying to change it, but that is not actually
            // what we want to do. We'll let the event handler manage it themselves because it's possible the window manager might do some
            // window-specific cursor management and the old and new elements are on different windows.
            drgui__change_cursor(pNewElementUnderMouse, newCursor);



            // The the event handlers below, remember that ancestors are considered hovered if a descendant is the element under the mouse.

            // on_mouse_leave
            drgui_post_on_mouse_leave_recursive(pContext, pNewElementUnderMouse, pOldElementUnderMouse);

            // on_mouse_enter
            drgui_post_on_mouse_enter_recursive(pContext, pNewElementUnderMouse, pOldElementUnderMouse);
        }
    }
}


void drgui_post_outbound_event_move(drgui_element* pElement, float newRelativePosX, float newRelativePosY)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onMove) {
            pElement->onMove(pElement, newRelativePosX, newRelativePosY);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_size(drgui_element* pElement, float newWidth, float newHeight)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onSize) {
            pElement->onSize(pElement, newWidth, newHeight);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_mouse_enter(drgui_element* pElement)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onMouseEnter) {
            pElement->onMouseEnter(pElement);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_mouse_leave(drgui_element* pElement)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onMouseLeave) {
            pElement->onMouseLeave(pElement);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_mouse_move(drgui_element* pElement, int relativeMousePosX, int relativeMousePosY, int stateFlags)
{
    if (drgui_begin_outbound_event(pElement))
{
        if (pElement->onMouseMove) {
            pElement->onMouseMove(pElement, relativeMousePosX, relativeMousePosY, stateFlags);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_mouse_button_down(drgui_element* pElement, int mouseButton, int relativeMousePosX, int relativeMousePosY, int stateFlags)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onMouseButtonDown) {
            pElement->onMouseButtonDown(pElement, mouseButton, relativeMousePosX, relativeMousePosY, stateFlags);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_mouse_button_up(drgui_element* pElement, int mouseButton, int relativeMousePosX, int relativeMousePosY, int stateFlags)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onMouseButtonUp) {
            pElement->onMouseButtonUp(pElement, mouseButton, relativeMousePosX, relativeMousePosY, stateFlags);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_mouse_button_dblclick(drgui_element* pElement, int mouseButton, int relativeMousePosX, int relativeMousePosY, int stateFlags)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onMouseButtonDblClick) {
            pElement->onMouseButtonDblClick(pElement, mouseButton, relativeMousePosX, relativeMousePosY, stateFlags);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_mouse_wheel(drgui_element* pElement, int delta, int relativeMousePosX, int relativeMousePosY, int stateFlags)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onMouseWheel) {
            pElement->onMouseWheel(pElement, delta, relativeMousePosX, relativeMousePosY, stateFlags);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_key_down(drgui_element* pElement, drgui_key key, int stateFlags)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onKeyDown) {
            pElement->onKeyDown(pElement, key, stateFlags);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_key_up(drgui_element* pElement, drgui_key key, int stateFlags)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onKeyUp) {
            pElement->onKeyUp(pElement, key, stateFlags);
        }

        drgui_end_outbound_event(pElement);
    }
}

void drgui_post_outbound_event_printable_key_down(drgui_element* pElement, unsigned int character, int stateFlags)
{
    if (drgui_begin_outbound_event(pElement))
    {
        if (pElement->onPrintableKeyDown) {
            pElement->onPrintableKeyDown(pElement, character, stateFlags);
        }

        drgui_end_outbound_event(pElement);
    }
}


void drgui_post_outbound_event_dirty(drgui_element* pElement, drgui_rect relativeRect)
{
    if (pElement != NULL)
    {
        if (pElement->onDirty) {
            pElement->onDirty(pElement, relativeRect);
        }
    }
}

void drgui_post_outbound_event_dirty_global(drgui_element* pElement, drgui_rect relativeRect)
{
    if (pElement != NULL && pElement->pContext != NULL)
    {
        if (pElement->pContext->onGlobalDirty) {
            pElement->pContext->onGlobalDirty(pElement, relativeRect);
        }
    }
}

void drgui_post_outbound_event_capture_mouse(drgui_element* pElement)
{
    if (pElement != NULL)
    {
        if (pElement->onCaptureMouse) {
            pElement->onCaptureMouse(pElement);
        }
    }
}

void drgui_post_outbound_event_capture_mouse_global(drgui_element* pElement)
{
    if (pElement != NULL && pElement->pContext != NULL)
    {
        if (pElement->pContext->onGlobalCaptureMouse) {
            pElement->pContext->onGlobalCaptureMouse(pElement);
        }
    }
}

void drgui_post_outbound_event_release_mouse(drgui_element* pElement)
{
    if (pElement != NULL)
    {
        if (pElement->onReleaseMouse) {
            pElement->onReleaseMouse(pElement);
        }
    }
}

void drgui_post_outbound_event_release_mouse_global(drgui_element* pElement)
{
    if (pElement != NULL && pElement->pContext != NULL)
    {
        if (pElement->pContext->onGlobalReleaseMouse) {
            pElement->pContext->onGlobalReleaseMouse(pElement);
        }
    }
}


void drgui_post_outbound_event_capture_keyboard(drgui_element* pElement, drgui_element* pPrevCapturedElement)
{
    if (pElement != NULL)
    {
        if (pElement->onCaptureKeyboard) {
            pElement->onCaptureKeyboard(pElement, pPrevCapturedElement);
        }
    }
}

void drgui_post_outbound_event_capture_keyboard_global(drgui_element* pElement, drgui_element* pPrevCapturedElement)
{
    if (pElement != NULL && pElement->pContext != NULL)
    {
        if (pElement->pContext->onGlobalCaptureKeyboard) {
            pElement->pContext->onGlobalCaptureKeyboard(pElement, pPrevCapturedElement);
        }
    }
}

void drgui_post_outbound_event_release_keyboard(drgui_element* pElement, drgui_element* pNewCapturedElement)
{
    if (pElement != NULL)
    {
        if (pElement->onReleaseKeyboard) {
            pElement->onReleaseKeyboard(pElement, pNewCapturedElement);
        }
    }
}

void drgui_post_outbound_event_release_keyboard_global(drgui_element* pElement, drgui_element* pNewCapturedElement)
{
    if (pElement != NULL && pElement->pContext != NULL)
    {
        if (pElement->pContext->onGlobalReleaseKeyboard) {
            pElement->pContext->onGlobalReleaseKeyboard(pElement, pNewCapturedElement);
        }
    }
}


void drgui_log(drgui_context* pContext, const char* message)
{
    if (pContext != NULL)
    {
        if (pContext->onLog) {
            pContext->onLog(pContext, message);
        }
    }
}


/////////////////////////////////////////////////////////////////
//
// CORE API
//
/////////////////////////////////////////////////////////////////

drgui_context* drgui_create_context()
{
    drgui_context* pContext = (drgui_context*)calloc(1, sizeof(drgui_context));
    if (pContext != NULL) {
        pContext->currentCursor = drgui_cursor_default;
    }

    return pContext;
}

void drgui_delete_context(drgui_context* pContext)
{
    if (pContext == NULL) {
        return;
    }


    // Make sure the mouse capture is released.
    if (pContext->pElementWithMouseCapture != NULL)
    {
        drgui_log(pContext, "WARNING: Deleting the GUI context while an element still has the mouse capture.");
        drgui_release_mouse(pContext);
    }

    // Make sure the keyboard capture is released.
    if (pContext->pElementWithKeyboardCapture != NULL)
    {
        drgui_log(pContext, "WARNING: Deleting the GUI context while an element still has the keyboard capture.");
        drgui_release_keyboard(pContext);
    }


    if (drgui_is_handling_inbound_event(pContext))
    {
        // An inbound event is still being processed - we don't want to delete the context straight away because we can't
        // trust external event handlers to not try to access the context later on. To do this we just set the flag that
        // the context is deleted. It will then be deleted for real at the end of the inbound event handler.
        drgui_mark_context_as_dead(pContext);
    }
    else
    {
        // An inbound event is not being processed, so delete the context straight away.
        drgui_delete_context_for_real(pContext);
    }
}



/////////////////////////////////////////////////////////////////
// Events

void drgui_post_inbound_event_mouse_leave(drgui_element* pTopLevelElement)
{
    if (pTopLevelElement == NULL) {
        return;
    }

    drgui_context* pContext = pTopLevelElement->pContext;
    if (pContext == NULL) {
        return;
    }

    drgui_begin_inbound_event(pContext);
    {
        // We assume that was previously under the mouse was either pTopLevelElement itself or one of it's descendants.
        drgui_update_mouse_enter_and_leave_state(pContext, NULL);
    }
    drgui_end_inbound_event(pContext);
}

void drgui_post_inbound_event_mouse_move(drgui_element* pTopLevelElement, int mousePosX, int mousePosY, int stateFlags)
{
    if (pTopLevelElement == NULL || pTopLevelElement->pContext == NULL) {
        return;
    }


    drgui_begin_inbound_event(pTopLevelElement->pContext);
    {
        /// A pointer to the top level element that was passed in from the last inbound mouse move event.
        pTopLevelElement->pContext->pLastMouseMoveTopLevelElement = pTopLevelElement;

        /// The position of the mouse that was passed in from the last inbound mouse move event.
        pTopLevelElement->pContext->lastMouseMovePosX = (float)mousePosX;
        pTopLevelElement->pContext->lastMouseMovePosY = (float)mousePosY;



        // The first thing we need to do is find the new element that's sitting under the mouse.
        drgui_element* pNewElementUnderMouse = drgui_find_element_under_point(pTopLevelElement, (float)mousePosX, (float)mousePosY);

        // Now that we know which element is sitting under the mouse we need to check if the mouse has entered into a new element.
        drgui_update_mouse_enter_and_leave_state(pTopLevelElement->pContext, pNewElementUnderMouse);


        drgui_element* pEventReceiver = pTopLevelElement->pContext->pElementWithMouseCapture;
        if (pEventReceiver == NULL)
        {
            pEventReceiver = pNewElementUnderMouse;
        }

        if (pEventReceiver != NULL)
        {
            float relativeMousePosX = (float)mousePosX;
            float relativeMousePosY = (float)mousePosY;
            drgui_make_point_relative(pEventReceiver, &relativeMousePosX, &relativeMousePosY);

            drgui_post_outbound_event_mouse_move(pEventReceiver, (int)relativeMousePosX, (int)relativeMousePosY, stateFlags);
        }
    }
    drgui_end_inbound_event(pTopLevelElement->pContext);
}

void drgui_post_inbound_event_mouse_button_down(drgui_element* pTopLevelElement, int mouseButton, int mousePosX, int mousePosY, int stateFlags)
{
    if (pTopLevelElement == NULL || pTopLevelElement->pContext == NULL) {
        return;
    }

    drgui_context* pContext = pTopLevelElement->pContext;
    drgui_begin_inbound_event(pContext);
    {
        drgui_element* pEventReceiver = pContext->pElementWithMouseCapture;
        if (pEventReceiver == NULL)
        {
            pEventReceiver = pContext->pElementUnderMouse;

            if (pEventReceiver == NULL)
            {
                // We'll get here if this message is posted without a prior mouse move event.
                pEventReceiver = drgui_find_element_under_point(pTopLevelElement, (float)mousePosX, (float)mousePosY);
            }
        }


        if (pEventReceiver != NULL)
        {
            float relativeMousePosX = (float)mousePosX;
            float relativeMousePosY = (float)mousePosY;
            drgui_make_point_relative(pEventReceiver, &relativeMousePosX, &relativeMousePosY);

            drgui_post_outbound_event_mouse_button_down(pEventReceiver, mouseButton, (int)relativeMousePosX, (int)relativeMousePosY, stateFlags);
        }
    }
    drgui_end_inbound_event(pContext);
}

void drgui_post_inbound_event_mouse_button_up(drgui_element* pTopLevelElement, int mouseButton, int mousePosX, int mousePosY, int stateFlags)
{
    if (pTopLevelElement == NULL || pTopLevelElement->pContext == NULL) {
        return;
    }

    drgui_context* pContext = pTopLevelElement->pContext;
    drgui_begin_inbound_event(pContext);
    {
        drgui_element* pEventReceiver = pContext->pElementWithMouseCapture;
        if (pEventReceiver == NULL)
        {
            pEventReceiver = pContext->pElementUnderMouse;

            if (pEventReceiver == NULL)
            {
                // We'll get here if this message is posted without a prior mouse move event.
                pEventReceiver = drgui_find_element_under_point(pTopLevelElement, (float)mousePosX, (float)mousePosY);
            }
        }


        if (pEventReceiver != NULL)
        {
            float relativeMousePosX = (float)mousePosX;
            float relativeMousePosY = (float)mousePosY;
            drgui_make_point_relative(pEventReceiver, &relativeMousePosX, &relativeMousePosY);

            drgui_post_outbound_event_mouse_button_up(pEventReceiver, mouseButton, (int)relativeMousePosX, (int)relativeMousePosY, stateFlags);
        }
    }
    drgui_end_inbound_event(pContext);
}

void drgui_post_inbound_event_mouse_button_dblclick(drgui_element* pTopLevelElement, int mouseButton, int mousePosX, int mousePosY, int stateFlags)
{
    if (pTopLevelElement == NULL || pTopLevelElement->pContext == NULL) {
        return;
    }

    drgui_context* pContext = pTopLevelElement->pContext;
    drgui_begin_inbound_event(pContext);
    {
        drgui_element* pEventReceiver = pContext->pElementWithMouseCapture;
        if (pEventReceiver == NULL)
        {
            pEventReceiver = pContext->pElementUnderMouse;

            if (pEventReceiver == NULL)
            {
                // We'll get here if this message is posted without a prior mouse move event.
                pEventReceiver = drgui_find_element_under_point(pTopLevelElement, (float)mousePosX, (float)mousePosY);
            }
        }


        if (pEventReceiver != NULL)
        {
            float relativeMousePosX = (float)mousePosX;
            float relativeMousePosY = (float)mousePosY;
            drgui_make_point_relative(pEventReceiver, &relativeMousePosX, &relativeMousePosY);

            drgui_post_outbound_event_mouse_button_dblclick(pEventReceiver, mouseButton, (int)relativeMousePosX, (int)relativeMousePosY, stateFlags);
        }
    }
    drgui_end_inbound_event(pContext);
}

void drgui_post_inbound_event_mouse_wheel(drgui_element* pTopLevelElement, int delta, int mousePosX, int mousePosY, int stateFlags)
{
    if (pTopLevelElement == NULL || pTopLevelElement->pContext == NULL) {
        return;
    }

    drgui_context* pContext = pTopLevelElement->pContext;
    drgui_begin_inbound_event(pContext);
    {
        drgui_element* pEventReceiver = pContext->pElementWithMouseCapture;
        if (pEventReceiver == NULL)
        {
            pEventReceiver = pContext->pElementUnderMouse;

            if (pEventReceiver == NULL)
            {
                // We'll get here if this message is posted without a prior mouse move event.
                pEventReceiver = drgui_find_element_under_point(pTopLevelElement, (float)mousePosX, (float)mousePosY);
            }
        }


        if (pEventReceiver != NULL)
        {
            float relativeMousePosX = (float)mousePosX;
            float relativeMousePosY = (float)mousePosY;
            drgui_make_point_relative(pEventReceiver, &relativeMousePosX, &relativeMousePosY);

            drgui_post_outbound_event_mouse_wheel(pEventReceiver, delta, (int)relativeMousePosX, (int)relativeMousePosY, stateFlags);
        }
    }
    drgui_end_inbound_event(pContext);
}

void drgui_post_inbound_event_key_down(drgui_context* pContext, drgui_key key, int stateFlags)
{
    if (pContext == NULL) {
        return;
    }

    drgui_begin_inbound_event(pContext);
    {
        if (pContext->pElementWithKeyboardCapture != NULL) {
            drgui_post_outbound_event_key_down(pContext->pElementWithKeyboardCapture, key, stateFlags);
        }
    }
    drgui_end_inbound_event(pContext);
}

void drgui_post_inbound_event_key_up(drgui_context* pContext, drgui_key key, int stateFlags)
{
    if (pContext == NULL) {
        return;
    }

    drgui_begin_inbound_event(pContext);
    {
        if (pContext->pElementWithKeyboardCapture != NULL) {
            drgui_post_outbound_event_key_up(pContext->pElementWithKeyboardCapture, key, stateFlags);
        }
    }
    drgui_end_inbound_event(pContext);
}

void drgui_post_inbound_event_printable_key_down(drgui_context* pContext, unsigned int character, int stateFlags)
{
    if (pContext == NULL) {
        return;
    }

    drgui_begin_inbound_event(pContext);
    {
        if (pContext->pElementWithKeyboardCapture != NULL) {
            drgui_post_outbound_event_printable_key_down(pContext->pElementWithKeyboardCapture, character, stateFlags);
        }
    }
    drgui_end_inbound_event(pContext);
}



void drgui_set_global_on_dirty(drgui_context * pContext, drgui_on_dirty_proc onDirty)
{
    if (pContext != NULL) {
        pContext->onGlobalDirty = onDirty;
    }
}

void drgui_set_global_on_capture_mouse(drgui_context* pContext, drgui_on_capture_mouse_proc onCaptureMouse)
{
    if (pContext != NULL) {
        pContext->onGlobalCaptureMouse = onCaptureMouse;
    }
}

void drgui_set_global_on_release_mouse(drgui_context* pContext, drgui_on_release_mouse_proc onReleaseMouse)
{
    if (pContext != NULL) {
        pContext->onGlobalReleaseMouse = onReleaseMouse;
    }
}

void drgui_set_global_on_capture_keyboard(drgui_context* pContext, drgui_on_capture_keyboard_proc onCaptureKeyboard)
{
    if (pContext != NULL) {
        pContext->onGlobalCaptureKeyboard = onCaptureKeyboard;
    }
}

void drgui_set_global_on_release_keyboard(drgui_context* pContext, drgui_on_capture_keyboard_proc onReleaseKeyboard)
{
    if (pContext != NULL) {
        pContext->onGlobalReleaseKeyboard = onReleaseKeyboard;
    }
}

void drgui_set_global_on_change_cursor(drgui_context* pContext, drgui_on_change_cursor_proc onChangeCursor)
{
    if (pContext != NULL) {
        pContext->onChangeCursor = onChangeCursor;
    }
}

void drgui_set_on_delete_element(drgui_context* pContext, drgui_on_delete_element_proc onDeleteElement)
{
    if (pContext != NULL) {
        pContext->onDeleteElement = onDeleteElement;
    }
}

void drgui_set_on_log(drgui_context* pContext, drgui_on_log onLog)
{
    if (pContext != NULL) {
        pContext->onLog = onLog;
    }
}



/////////////////////////////////////////////////////////////////
// Elements

drgui_element* drgui_create_element(drgui_context* pContext, drgui_element* pParent, size_t extraDataSize, const void* pExtraData)
{
    if (pContext != NULL)
    {
        drgui_element* pElement = (drgui_element*)calloc(1, sizeof(drgui_element) + extraDataSize);
        if (pElement != NULL) {
            pElement->pContext = pContext;
            pElement->pParent = pParent;
            pElement->cursor = drgui_cursor_default;
            pElement->dirtyRect = drgui_make_inside_out_rect();

            pElement->extraDataSize = extraDataSize;
            if (pExtraData != NULL) {
                memcpy(pElement->pExtraData, pExtraData, extraDataSize);
            }

            // Add to the the hierarchy.
            drgui_append_without_detach_or_redraw(pElement, pElement->pParent);


            // Have the element positioned at 0,0 relative to the parent by default.
            if (pParent != NULL) {
                pElement->absolutePosX = pParent->absolutePosX;
                pElement->absolutePosY = pParent->absolutePosY;
            }


            return pElement;
        }
    }

    return NULL;
}

void drgui_delete_element(drgui_element* pElement)
{
    if (pElement == NULL) {
        return;
    }

    drgui_context* pContext = pElement->pContext;
    if (pContext == NULL) {
        return;
    }

    if (drgui_is_element_marked_as_dead(pElement)) {
        drgui_log(pContext, "WARNING: Attempting to delete an element that is already marked for deletion.");
        return;
    }

    drgui_mark_element_as_dead(pElement);


    // Notify the application that the element is being deleted. Do this at the top so the event handler can access things like the hierarchy and
    // whatnot in case it needs it.
    if (pContext->onDeleteElement) {
        pContext->onDeleteElement(pElement);
    }


    // If this was element is marked as the one that was last under the mouse it needs to be unset.
    bool needsMouseUpdate = false;
    if (pContext->pElementUnderMouse == pElement)
    {
        pContext->pElementUnderMouse = NULL;
        needsMouseUpdate = true;
    }

    if (pContext->pLastMouseMoveTopLevelElement == pElement)
    {
        pContext->pLastMouseMoveTopLevelElement = NULL;
        pContext->lastMouseMovePosX = 0;
        pContext->lastMouseMovePosY = 0;
        needsMouseUpdate = false;       // It was a top-level element so the mouse enter/leave state doesn't need an update.
    }


    // If this element has the mouse capture it needs to be released.
    if (pContext->pElementWithMouseCapture == pElement)
    {
        drgui_log(pContext, "WARNING: Deleting an element while it still has the mouse capture.");
        drgui_release_mouse(pContext);
    }

    // If this element has the keyboard capture it needs to be released.
    if (pContext->pElementWithKeyboardCapture == pElement)
    {
        drgui_log(pContext, "WARNING: Deleting an element while it still has the keyboard capture.");
        drgui_release_keyboard(pContext);
    }

    // Is this element in the middle of being marked as dirty?
    for (size_t iDirtyElement = 0; iDirtyElement < pContext->dirtyElementCount; ++iDirtyElement) {
        if (pContext->ppDirtyElements[iDirtyElement] == pElement) {
            drgui_log(pContext, "WARNING: Deleting an element while it is being marked as dirty.");
            for (size_t iDirtyElement2 = iDirtyElement; iDirtyElement2+1 < pContext->dirtyElementCount; ++iDirtyElement2) {
                pContext->ppDirtyElements[iDirtyElement2] = pContext->ppDirtyElements[iDirtyElement2+1];
            }

            pContext->dirtyElementCount -= 1;
            break;
        }
    }

#if 0
    if (pContext->pDirtyTopLevelElement == pElement)
    {
        drgui_log(pContext, "WARNING: Deleting an element while it is being marked as dirty.");
        pContext->pDirtyTopLevelElement = NULL;
    }
#endif



    // Deleting this element may have resulted in the mouse entering a new element. Here is where we do a mouse enter/leave update.
    if (needsMouseUpdate)
    {
        pElement->onHitTest = drgui_pass_through_hit_test;        // <-- This ensures we don't include this element when searching for the new element under the mouse.
        drgui_update_mouse_enter_and_leave_state(pContext, drgui_find_element_under_point(pContext->pLastMouseMoveTopLevelElement, pContext->lastMouseMovePosX, pContext->lastMouseMovePosY));
    }


    // Here is where we need to detach the element from the hierarchy. When doing this we want to ensure the element is not redrawn when
    // it's children are detached. To do this we simply detach the event handler.
    pElement->onPaint = NULL;

    // The parent needs to be redraw after detaching.
    drgui_element* pParent = pElement->pParent;
    drgui_rect relativeRect = drgui_get_relative_rect(pElement);


    // Orphan the element first.
    drgui_detach_without_redraw(pElement);

    // Children need to be deleted before deleting the element itself.
    while (pElement->pLastChild != NULL) {
        drgui_delete_element(pElement->pLastChild);
    }


    // The parent needs to be redrawn.
    if (pParent) {
        drgui_dirty(pParent, relativeRect);
    }


    // Finally, we to decided whether or not the element should be deleted for real straight away or not. If the element is being
    // deleted within an event handler it should be delayed because the event handlers may try referencing it afterwards.
    if (!drgui_is_handling_inbound_event(pContext)) {
        drgui_delete_element_for_real(pElement);
    }
}


size_t drgui_get_extra_data_size(drgui_element* pElement)
{
    if (pElement != NULL) {
        return pElement->extraDataSize;
    }

    return 0;
}

void* drgui_get_extra_data(drgui_element* pElement)
{
    if (pElement != NULL) {
        return pElement->pExtraData;
    }

    return NULL;
}


bool drgui_set_type(drgui_element* pElement, const char* type)
{
    if (pElement == NULL) {
        return false;
    }

    return drgui__strcpy_s(pElement->type, sizeof(pElement->type), (type == NULL) ? "" : type) == 0;
}

const char* drgui_get_type(drgui_element* pElement)
{
    if (pElement == NULL) {
        return NULL;
    }

    return pElement->type;
}

bool drgui_is_of_type(drgui_element* pElement, const char* type)
{
    if (pElement == NULL || type == NULL) {
        return false;
    }

    return strncmp(pElement->type, type, strlen(type)) == 0;
}


void drgui_hide(drgui_element* pElement)
{
    if (pElement != NULL) {
        pElement->flags |= IS_ELEMENT_HIDDEN;
        drgui_auto_dirty(pElement, drgui_get_local_rect(pElement));
    }
}

void drgui_show(drgui_element* pElement)
{
    if (pElement != NULL) {
        pElement->flags &= ~IS_ELEMENT_HIDDEN;
        drgui_auto_dirty(pElement, drgui_get_local_rect(pElement));
    }
}

bool drgui_is_visible(const drgui_element* pElement)
{
    if (pElement != NULL) {
        return (pElement->flags & IS_ELEMENT_HIDDEN) == 0;
    }

    return false;
}

bool drgui_is_visible_recursive(const drgui_element* pElement)
{
    if (drgui_is_visible(pElement))
    {
        assert(pElement->pParent != NULL);

        if (pElement->pParent != NULL) {
            return drgui_is_visible(pElement->pParent);
        }
    }

    return false;
}


void drgui_disable_clipping(drgui_element* pElement)
{
    if (pElement != NULL) {
        pElement->flags |= IS_ELEMENT_CLIPPING_DISABLED;
    }
}

void drgui_enable_clipping(drgui_element* pElement)
{
    if (pElement != NULL) {
        pElement->flags &= ~IS_ELEMENT_CLIPPING_DISABLED;
    }
}

bool drgui_is_clipping_enabled(const drgui_element* pElement)
{
    if (pElement != NULL) {
        return (pElement->flags & IS_ELEMENT_CLIPPING_DISABLED) == 0;
    }

    return true;
}



void drgui_capture_mouse(drgui_element* pElement)
{
    if (pElement == NULL) {
        return;
    }

    if (pElement->pContext == NULL) {
        return;
    }


    if (pElement->pContext->pElementWithMouseCapture != pElement)
    {
        // Release the previous capture first.
        if (pElement->pContext->pElementWithMouseCapture != NULL) {
            drgui_release_mouse(pElement->pContext);
        }

        assert(pElement->pContext->pElementWithMouseCapture == NULL);

        pElement->pContext->pElementWithMouseCapture = pElement;

        // Two events need to be posted - the global on_capture_mouse event and the local on_capture_mouse event.
        drgui_post_outbound_event_capture_mouse(pElement);

        if (pElement == pElement->pContext->pElementWithMouseCapture) {  // <-- Only post the global event handler if the element still has the capture.
            drgui_post_outbound_event_capture_mouse_global(pElement);
        }
    }
}

void drgui_release_mouse(drgui_context* pContext)
{
    if (pContext == NULL) {
        return;
    }


    // Events need to be posted before setting the internal pointer.
    if (!drgui_is_element_marked_as_dead(pContext->pElementWithMouseCapture)) {   // <-- There's a chace the element is releaseing the keyboard due to being deleted. Don't want to post an event in this case.
        drgui_post_outbound_event_release_mouse(pContext->pElementWithMouseCapture);
        drgui_post_outbound_event_release_mouse_global(pContext->pElementWithMouseCapture);
    }

    // We want to set the internal pointer to NULL after posting the events since that is when it has truly released the mouse.
    pContext->pElementWithMouseCapture = NULL;


    // After releasing the mouse the cursor may be sitting on top of a different element - we want to recheck that.
    drgui_update_mouse_enter_and_leave_state(pContext, drgui_find_element_under_point(pContext->pLastMouseMoveTopLevelElement, pContext->lastMouseMovePosX, pContext->lastMouseMovePosY));
}

void drgui_release_mouse_no_global_notify(drgui_context* pContext)
{
    if (pContext == NULL) {
        return;
    }

    drgui_on_release_mouse_proc prevProc = pContext->onGlobalReleaseMouse;
    pContext->onGlobalReleaseMouse = NULL;
    drgui_release_mouse(pContext);
    pContext->onGlobalReleaseMouse = prevProc;
}

drgui_element* drgui_get_element_with_mouse_capture(drgui_context* pContext)
{
    if (pContext == NULL) {
        return NULL;
    }

    return pContext->pElementWithMouseCapture;
}

bool drgui_has_mouse_capture(drgui_element* pElement)
{
    if (pElement == NULL) {
        return false;
    }

    return drgui_get_element_with_mouse_capture(pElement->pContext) == pElement;
}


DRGUI_PRIVATE void drgui_release_keyboard_private(drgui_context* pContext, drgui_element* pNewCapturedElement)
{
    assert(pContext != NULL);

    // It is reasonable to expect that an application will want to change keyboard focus from within the release_keyboard
    // event handler. The problem with this is that is can cause a infinite dependency chain. We need to handle that case
    // by setting a flag that keeps track of whether or not we are in the middle of a release_keyboard event. At the end
    // we look at the element that want's the keyboard focuse and explicitly capture it at the end.

    pContext->flags |= IS_RELEASING_KEYBOARD;
    {
        drgui_element* pPrevCapturedElement = pContext->pElementWithKeyboardCapture;
        pContext->pElementWithKeyboardCapture = NULL;

        if (!drgui_is_element_marked_as_dead(pPrevCapturedElement)) {   // <-- There's a chace the element is releaseing the keyboard due to being deleted. Don't want to post an event in this case.
            drgui_post_outbound_event_release_keyboard(pPrevCapturedElement, pNewCapturedElement);
            drgui_post_outbound_event_release_keyboard_global(pPrevCapturedElement, pNewCapturedElement);
        }
    }
    pContext->flags &= ~IS_RELEASING_KEYBOARD;

    // Explicitly capture the keyboard.
    drgui_capture_keyboard(pContext->pElementWantingKeyboardCapture);
    pContext->pElementWantingKeyboardCapture = NULL;
}

void drgui_capture_keyboard(drgui_element* pElement)
{
    if (pElement == NULL) {
        return;
    }

    if (pElement->pContext == NULL) {
        return;
    }


    if ((pElement->pContext->flags & IS_RELEASING_KEYBOARD) != 0) {
        pElement->pContext->pElementWantingKeyboardCapture = pElement;
        return;
    }


    if (pElement->pContext->pElementWithKeyboardCapture != pElement)
    {
        // Release the previous capture first.
        drgui_element* pPrevElementWithKeyboardCapture = pElement->pContext->pElementWithKeyboardCapture;
        if (pPrevElementWithKeyboardCapture != NULL) {
            drgui_release_keyboard_private(pElement->pContext, pElement);
        }

        assert(pElement->pContext->pElementWithKeyboardCapture == NULL);

        pElement->pContext->pElementWithKeyboardCapture = pElement;
        pElement->pContext->pElementWantingKeyboardCapture = NULL;

        // Two events need to be posted - the global on_capture event and the local on_capture event. The problem, however, is that the
        // local event handler may change the keyboard capture internally, such as if it wants to pass it's focus onto an internal child
        // element or whatnot. In this case we don't want to fire the global event handler because it will result in superfluous event
        // posting, and could also be posted with an incorrect element.
        drgui_post_outbound_event_capture_keyboard(pElement, pPrevElementWithKeyboardCapture);

        if (pElement == pElement->pContext->pElementWithKeyboardCapture) {  // <-- Only post the global event handler if the element still has the capture.
            drgui_post_outbound_event_capture_keyboard_global(pElement, pPrevElementWithKeyboardCapture);
        }
    }
}

void drgui_release_keyboard(drgui_context* pContext)
{
    if (pContext == NULL) {
        return;
    }

    drgui_release_keyboard_private(pContext, NULL);
}

void drgui_release_keyboard_no_global_notify(drgui_context* pContext)
{
    if (pContext == NULL) {
        return;
    }

    drgui_on_release_keyboard_proc prevProc = pContext->onGlobalReleaseKeyboard;
    pContext->onGlobalReleaseKeyboard = NULL;
    drgui_release_keyboard(pContext);
    pContext->onGlobalReleaseKeyboard = prevProc;
}

drgui_element* drgui_get_element_with_keyboard_capture(drgui_context* pContext)
{
    if (pContext == NULL) {
        return NULL;
    }

    return pContext->pElementWithKeyboardCapture;
}

bool drgui_has_keyboard_capture(drgui_element* pElement)
{
    if (pElement == NULL) {
        return false;
    }

    return drgui_get_element_with_keyboard_capture(pElement->pContext) == pElement;
}


void drgui_set_cursor(drgui_element* pElement, drgui_cursor_type cursor)
{
    if (pElement == NULL) {
        return;
    }

    pElement->cursor = cursor;

    if (drgui_is_element_under_mouse(pElement) && pElement->pContext->currentCursor != cursor) {
        drgui__change_cursor(pElement, cursor);
    }
}

drgui_cursor_type drgui_get_cursor(drgui_element* pElement)
{
    if (pElement == NULL) {
        return drgui_cursor_none;
    }

    return pElement->cursor;
}



//// Events ////

void drgui_set_on_move(drgui_element * pElement, drgui_on_move_proc callback)
{
    if (pElement != NULL) {
        pElement->onMove = callback;
    }
}

void drgui_set_on_size(drgui_element * pElement, drgui_on_size_proc callback)
{
    if (pElement != NULL) {
        pElement->onSize = callback;
    }
}

void drgui_set_on_mouse_enter(drgui_element* pElement, drgui_on_mouse_enter_proc callback)
{
    if (pElement != NULL) {
        pElement->onMouseEnter = callback;
    }
}

void drgui_set_on_mouse_leave(drgui_element* pElement, drgui_on_mouse_leave_proc callback)
{
    if (pElement != NULL) {
        pElement->onMouseLeave = callback;
    }
}

void drgui_set_on_mouse_move(drgui_element* pElement, drgui_on_mouse_move_proc callback)
{
    if (pElement != NULL) {
        pElement->onMouseMove = callback;
    }
}

void drgui_set_on_mouse_button_down(drgui_element* pElement, drgui_on_mouse_button_down_proc callback)
{
    if (pElement != NULL) {
        pElement->onMouseButtonDown = callback;
    }
}

void drgui_set_on_mouse_button_up(drgui_element* pElement, drgui_on_mouse_button_up_proc callback)
{
    if (pElement != NULL) {
        pElement->onMouseButtonUp = callback;
    }
}

void drgui_set_on_mouse_button_dblclick(drgui_element* pElement, drgui_on_mouse_button_dblclick_proc callback)
{
    if (pElement != NULL) {
        pElement->onMouseButtonDblClick = callback;
    }
}

void drgui_set_on_mouse_wheel(drgui_element* pElement, drgui_on_mouse_wheel_proc callback)
{
    if (pElement != NULL) {
        pElement->onMouseWheel = callback;
    }
}

void drgui_set_on_key_down(drgui_element* pElement, drgui_on_key_down_proc callback)
{
    if (pElement != NULL) {
        pElement->onKeyDown = callback;
    }
}

void drgui_set_on_key_up(drgui_element* pElement, drgui_on_key_up_proc callback)
{
    if (pElement != NULL) {
        pElement->onKeyUp = callback;
    }
}

void drgui_set_on_printable_key_down(drgui_element* pElement, drgui_on_printable_key_down_proc callback)
{
    if (pElement != NULL) {
        pElement->onPrintableKeyDown = callback;
    }
}

void drgui_set_on_paint(drgui_element* pElement, drgui_on_paint_proc callback)
{
    if (pElement != NULL) {
        pElement->onPaint = callback;
    }
}

void drgui_set_on_dirty(drgui_element * pElement, drgui_on_dirty_proc callback)
{
    if (pElement != NULL) {
        pElement->onDirty = callback;
    }
}

void drgui_set_on_hittest(drgui_element* pElement, drgui_on_hittest_proc callback)
{
    if (pElement != NULL) {
        pElement->onHitTest = callback;
    }
}

void drgui_set_on_capture_mouse(drgui_element* pElement, drgui_on_capture_mouse_proc callback)
{
    if (pElement != NULL) {
        pElement->onCaptureMouse = callback;
    }
}

void drgui_set_on_release_mouse(drgui_element* pElement, drgui_on_release_mouse_proc callback)
{
    if (pElement != NULL) {
        pElement->onReleaseMouse = callback;
    }
}

void drgui_set_on_capture_keyboard(drgui_element* pElement, drgui_on_capture_keyboard_proc callback)
{
    if (pElement != NULL) {
        pElement->onCaptureKeyboard = callback;
    }
}

void drgui_set_on_release_keyboard(drgui_element* pElement, drgui_on_release_keyboard_proc callback)
{
    if (pElement != NULL) {
        pElement->onReleaseKeyboard = callback;
    }
}



bool drgui_is_point_inside_element_bounds(const drgui_element* pElement, float absolutePosX, float absolutePosY)
{
    if (absolutePosX < pElement->absolutePosX ||
        absolutePosX < pElement->absolutePosY)
    {
        return false;
    }

    if (absolutePosX >= pElement->absolutePosX + pElement->width ||
        absolutePosY >= pElement->absolutePosY + pElement->height)
    {
        return false;
    }

    return true;
}

bool drgui_is_point_inside_element(drgui_element* pElement, float absolutePosX, float absolutePosY)
{
    if (drgui_is_point_inside_element_bounds(pElement, absolutePosX, absolutePosY))
    {
        // It is valid for onHitTest to be null, in which case we use the default hit test which assumes the element is just a rectangle
        // equal to the size of it's bounds. It's equivalent to onHitTest always returning true.

        if (pElement->onHitTest) {
            return pElement->onHitTest(pElement, absolutePosX - pElement->absolutePosX, absolutePosY - pElement->absolutePosY);
        }

        return true;
    }

    return false;
}



typedef struct
{
    drgui_element* pElementUnderPoint;
    float absolutePosX;
    float absolutePosY;
}drgui_find_element_under_point_data;

bool drgui_find_element_under_point_iterator(drgui_element* pElement, drgui_rect* pRelativeVisibleRect, void* pUserData)
{
    assert(pElement             != NULL);
    assert(pRelativeVisibleRect != NULL);

    drgui_find_element_under_point_data* pData = (drgui_find_element_under_point_data*)pUserData;
    assert(pData != NULL);

    float relativePosX = pData->absolutePosX;
    float relativePosY = pData->absolutePosY;
    drgui_make_point_relative(pElement, &relativePosX, &relativePosY);

    if (drgui_rect_contains_point(*pRelativeVisibleRect, relativePosX, relativePosY))
    {
        if (pElement->onHitTest) {
            if (pElement->onHitTest(pElement, relativePosX, relativePosY)) {
                pData->pElementUnderPoint = pElement;
            }
        } else {
            pData->pElementUnderPoint = pElement;
        }
    }


    // Always return true to ensure the entire hierarchy is checked.
    return true;
}

drgui_element* drgui_find_element_under_point(drgui_element* pTopLevelElement, float absolutePosX, float absolutePosY)
{
    if (pTopLevelElement == NULL) {
        return NULL;
    }

    drgui_find_element_under_point_data data;
    data.pElementUnderPoint = NULL;
    data.absolutePosX = absolutePosX;
    data.absolutePosY = absolutePosY;
    drgui_iterate_visible_elements(pTopLevelElement, drgui_get_absolute_rect(pTopLevelElement), drgui_find_element_under_point_iterator, &data);

    return data.pElementUnderPoint;
}

bool drgui_is_element_under_mouse(drgui_element* pElement)
{
    if (pElement == NULL) {
        return false;
    }

    return drgui_find_element_under_point(pElement->pContext->pLastMouseMoveTopLevelElement, pElement->pContext->lastMouseMovePosX, pElement->pContext->lastMouseMovePosY) == pElement;
}



//// Hierarchy ////

drgui_element* drgui_get_parent(drgui_element* pChildElement)
{
    if (pChildElement == NULL) {
        return NULL;
    }

    return pChildElement->pParent;
}

void drgui_detach(drgui_element* pChildElement)
{
    if (pChildElement == NULL) {
        return;
    }

    drgui_element* pOldParent = pChildElement->pParent;


    // We orphan the element using the private API. This will not mark the parent element as dirty so we need to do that afterwards.
    drgui_detach_without_redraw(pChildElement);

    // The region of the old parent needs to be redrawn.
    if (pOldParent != NULL) {
        drgui_auto_dirty(pOldParent, drgui_get_relative_rect(pOldParent));
    }
}

void drgui_append(drgui_element* pChildElement, drgui_element* pParentElement)
{
    if (pChildElement == NULL) {
        return;
    }

    // We first need to orphan the element. If the parent element is the new parent is the same as the old one, as in we
    // are just moving the child element to the end of the children list, we want to delay the repaint until the end. To
    // do this we use drgui_detach_without_redraw() because that will not trigger a redraw.
    if (pChildElement->pParent != pParentElement) {
        drgui_detach(pChildElement);
    } else {
        drgui_detach_without_redraw(pChildElement);
    }


    // Now we attach it to the end of the new parent.
    if (pParentElement != NULL) {
        drgui_append_without_detach(pChildElement, pParentElement);
    }
}

void drgui_prepend(drgui_element* pChildElement, drgui_element* pParentElement)
{
    if (pChildElement == NULL) {
        return;
    }

    // See comment in drgui_append() for explanation on this.
    if (pChildElement->pParent != pParentElement) {
        drgui_detach(pChildElement);
    } else {
        drgui_detach_without_redraw(pChildElement);
    }


    // Now we need to attach the element to the beginning of the parent.
    if (pParentElement != NULL) {
        drgui_prepend_without_detach(pChildElement, pParentElement);
    }
}

void drgui_append_sibling(drgui_element* pElementToAppend, drgui_element* pElementToAppendTo)
{
    if (pElementToAppend == NULL || pElementToAppendTo == NULL) {
        return;
    }

    // See comment in drgui_append() for explanation on this.
    if (pElementToAppend->pParent != pElementToAppendTo->pParent) {
        drgui_detach(pElementToAppend);
    } else {
        drgui_detach_without_redraw(pElementToAppend);
    }


    // Now we need to attach the element such that it comes just after pElementToAppendTo
    drgui_append_sibling_without_detach(pElementToAppend, pElementToAppendTo);
}

void drgui_prepend_sibling(drgui_element* pElementToPrepend, drgui_element* pElementToPrependTo)
{
    if (pElementToPrepend == NULL || pElementToPrependTo == NULL) {
        return;
    }

    // See comment in drgui_append() for explanation on this.
    if (pElementToPrepend->pParent != pElementToPrependTo->pParent) {
        drgui_detach(pElementToPrepend);
    } else {
        drgui_detach_without_redraw(pElementToPrepend);
    }


    // Now we need to attach the element such that it comes just after pElementToPrependTo
    drgui_prepend_sibling_without_detach(pElementToPrepend, pElementToPrependTo);
}

drgui_element* drgui_find_top_level_element(drgui_element* pElement)
{
    if (pElement == NULL) {
        return NULL;
    }

    if (pElement->pParent != NULL) {
        return drgui_find_top_level_element(pElement->pParent);
    }

    return pElement;
}

bool drgui_is_parent(drgui_element* pParentElement, drgui_element* pChildElement)
{
    if (pParentElement == NULL || pChildElement == NULL) {
        return false;
    }

    return pParentElement == pChildElement->pParent;
}

bool drgui_is_child(drgui_element* pChildElement, drgui_element* pParentElement)
{
    return drgui_is_parent(pParentElement, pChildElement);
}

bool drgui_is_ancestor(drgui_element* pAncestorElement, drgui_element* pChildElement)
{
    if (pAncestorElement == NULL || pChildElement == NULL) {
        return false;
    }

    drgui_element* pParent = pChildElement->pParent;
    while (pParent != NULL)
    {
        if (pParent == pAncestorElement) {
            return true;
        }

        pParent = pParent->pParent;
    }


    return false;
}

bool drgui_is_descendant(drgui_element* pChildElement, drgui_element* pAncestorElement)
{
    return drgui_is_ancestor(pAncestorElement, pChildElement);
}

bool drgui_is_self_or_ancestor(drgui_element* pAncestorElement, drgui_element* pChildElement)
{
    return pAncestorElement == pChildElement || drgui_is_ancestor(pAncestorElement, pChildElement);
}

bool drgui_is_self_or_descendant(drgui_element* pChildElement, drgui_element* pAncestorElement)
{
    return pChildElement == pAncestorElement || drgui_is_descendant(pChildElement, pAncestorElement);
}



//// Layout ////

void drgui_set_absolute_position(drgui_element* pElement, float positionX, float positionY)
{
    if (pElement != NULL)
    {
        if (pElement->absolutePosX != positionX || pElement->absolutePosY != positionY)
        {
            float oldRelativePosX = drgui_get_relative_position_x(pElement);
            float oldRelativePosY = drgui_get_relative_position_y(pElement);

            drgui_begin_auto_dirty(pElement);
            {
                drgui_auto_dirty(pElement, drgui_get_local_rect(pElement));     // <-- Previous rectangle.

                float offsetX = positionX - pElement->absolutePosX;
                float offsetY = positionY - pElement->absolutePosY;

                pElement->absolutePosX = positionX;
                pElement->absolutePosY = positionY;
                drgui_auto_dirty(pElement, drgui_get_local_rect(pElement));     // <-- New rectangle.


                float newRelativePosX = drgui_get_relative_position_x(pElement);
                float newRelativePosY = drgui_get_relative_position_y(pElement);

                if (newRelativePosX != oldRelativePosX || newRelativePosY != oldRelativePosY) {
                    drgui_post_outbound_event_move(pElement, newRelativePosX, newRelativePosY);
                }


                drgui_apply_offset_to_children_recursive(pElement, offsetX, offsetY);
            }
            drgui_end_auto_dirty(pElement);
        }
    }
}

void drgui_get_absolute_position(const drgui_element* pElement, float * positionXOut, float * positionYOut)
{
    if (pElement != NULL)
    {
        if (positionXOut != NULL) {
            *positionXOut = pElement->absolutePosX;
        }

        if (positionYOut != NULL) {
            *positionYOut = pElement->absolutePosY;
        }
    }
}

float drgui_get_absolute_position_x(const drgui_element* pElement)
{
    if (pElement != NULL) {
        return pElement->absolutePosX;
    }

    return 0.0f;
}

float drgui_get_absolute_position_y(const drgui_element* pElement)
{
    if (pElement != NULL) {
        return pElement->absolutePosY;
    }

    return 0.0f;
}


void drgui_set_relative_position(drgui_element* pElement, float relativePosX, float relativePosY)
{
    if (pElement != NULL) {
        if (pElement->pParent != NULL) {
            drgui_set_absolute_position(pElement, pElement->pParent->absolutePosX + relativePosX, pElement->pParent->absolutePosY + relativePosY);
        } else {
            drgui_set_absolute_position(pElement, relativePosX, relativePosY);
        }
    }
}

void drgui_get_relative_position(const drgui_element* pElement, float* positionXOut, float* positionYOut)
{
    if (pElement != NULL)
    {
        if (pElement->pParent != NULL)
        {
            if (positionXOut != NULL) {
                *positionXOut = pElement->absolutePosX - pElement->pParent->absolutePosX;
            }

            if (positionYOut != NULL) {
                *positionYOut = pElement->absolutePosY - pElement->pParent->absolutePosY;
            }
        }
        else
        {
            if (positionXOut != NULL) {
                *positionXOut = pElement->absolutePosX;
            }

            if (positionYOut != NULL) {
                *positionYOut = pElement->absolutePosY;
            }
        }
    }
}

float drgui_get_relative_position_x(const drgui_element* pElement)
{
    if (pElement != NULL) {
        if (pElement->pParent != NULL) {
            return pElement->absolutePosX - pElement->pParent->absolutePosX;
        } else {
            return pElement->absolutePosX;
        }
    }

    return 0;
}

float drgui_get_relative_position_y(const drgui_element* pElement)
{
    if (pElement != NULL) {
        if (pElement->pParent != NULL) {
            return pElement->absolutePosY - pElement->pParent->absolutePosY;
        } else {
            return pElement->absolutePosY;
        }
    }

    return 0;
}


void drgui_set_size(drgui_element* pElement, float width, float height)
{
    if (pElement != NULL)
    {
        if (pElement->width != width || pElement->height != height)
        {
            drgui_begin_auto_dirty(pElement);
            {
                drgui_auto_dirty(pElement, drgui_get_local_rect(pElement));     // <-- Previous rectangle.

                pElement->width  = width;
                pElement->height = height;
                drgui_auto_dirty(pElement, drgui_get_local_rect(pElement));     // <-- New rectangle.

                drgui_post_outbound_event_size(pElement, width, height);
            }
            drgui_end_auto_dirty(pElement);
        }
    }
}

void drgui_get_size(const drgui_element* pElement, float* widthOut, float* heightOut)
{
    if (pElement != NULL) {
        if (widthOut) *widthOut = pElement->width;
        if (heightOut) *heightOut = pElement->height;
    } else {
        if (widthOut) *widthOut = 0;
        if (heightOut) *heightOut = 0;
    }
}

float drgui_get_width(const drgui_element * pElement)
{
    if (pElement != NULL) {
        return pElement->width;
    }

    return 0;
}

float drgui_get_height(const drgui_element * pElement)
{
    if (pElement != NULL) {
        return pElement->height;
    }

    return 0;
}


drgui_rect drgui_get_absolute_rect(const drgui_element* pElement)
{
    drgui_rect rect;
    if (pElement != NULL)
    {
        rect.left   = pElement->absolutePosX;
        rect.top    = pElement->absolutePosY;
        rect.right  = rect.left + pElement->width;
        rect.bottom = rect.top + pElement->height;
    }
    else
    {
        rect.left   = 0;
        rect.top    = 0;
        rect.right  = 0;
        rect.bottom = 0;
    }

    return rect;
}

drgui_rect drgui_get_relative_rect(const drgui_element* pElement)
{
    drgui_rect rect;
    if (pElement != NULL)
    {
        rect.left   = drgui_get_relative_position_x(pElement);
        rect.top    = drgui_get_relative_position_y(pElement);
        rect.right  = rect.left + pElement->width;
        rect.bottom = rect.top  + pElement->height;
    }
    else
    {
        rect.left   = 0;
        rect.top    = 0;
        rect.right  = 0;
        rect.bottom = 0;
    }

    return rect;
}

drgui_rect drgui_get_local_rect(const drgui_element* pElement)
{
    drgui_rect rect;
    rect.left = 0;
    rect.top  = 0;

    if (pElement != NULL)
    {
        rect.right  = pElement->width;
        rect.bottom = pElement->height;
    }
    else
    {
        rect.right  = 0;
        rect.bottom = 0;
    }

    return rect;
}



//// Painting ////

bool drgui_register_painting_callbacks(drgui_context* pContext, void* pPaintingContext, drgui_painting_callbacks callbacks)
{
    if (pContext == NULL) {
        return false;
    }

    // Fail if the painting callbacks have already been registered.
    if (pContext->pPaintingContext != NULL) {
        return false;
    }


    pContext->pPaintingContext  = pPaintingContext;
    pContext->paintingCallbacks = callbacks;

    return true;
}


bool drgui_iterate_visible_elements(drgui_element* pParentElement, drgui_rect relativeRect, drgui_visible_iteration_proc callback, void* pUserData)
{
    if (pParentElement == NULL) {
        return false;
    }

    if (callback == NULL) {
        return false;
    }


    if (!drgui_is_visible(pParentElement)) {
        return true;
    }

    drgui_rect clampedRelativeRect = relativeRect;
    if (drgui_clamp_rect_to_element(pParentElement, &clampedRelativeRect))
    {
        // We'll only get here if some part of the rectangle was inside the element.
        if (!callback(pParentElement, &clampedRelativeRect, pUserData)) {
            return false;
        }
    }

    for (drgui_element* pChild = pParentElement->pFirstChild; pChild != NULL; pChild = pChild->pNextSibling)
    {
        float childRelativePosX = drgui_get_relative_position_x(pChild);
        float childRelativePosY = drgui_get_relative_position_y(pChild);

        drgui_rect childRect;
        if (drgui_is_clipping_enabled(pChild)) {
            childRect = clampedRelativeRect;
        } else {
            childRect = relativeRect;
        }


        childRect.left   -= childRelativePosX;
        childRect.top    -= childRelativePosY;
        childRect.right  -= childRelativePosX;
        childRect.bottom -= childRelativePosY;

        if (!drgui_iterate_visible_elements(pChild, childRect, callback, pUserData)) {
            return false;
        }
    }


    return true;
}

void drgui_disable_auto_dirty(drgui_context* pContext)
{
    if (pContext != NULL) {
        pContext->flags |= IS_AUTO_DIRTY_DISABLED;
    }
}

void drgui_enable_auto_dirty(drgui_context* pContext)
{
    if (pContext != NULL) {
        pContext->flags &= ~IS_AUTO_DIRTY_DISABLED;
    }
}

bool drgui_is_auto_dirty_enabled(drgui_context* pContext)
{
    if (pContext != NULL) {
        return (pContext->flags & IS_AUTO_DIRTY_DISABLED) == 0;
    }

    return false;
}


drgui_element* drgui_begin_dirty(drgui_element* pElement)
{
    if (pElement == NULL) {
        return NULL;
    }

    drgui_context* pContext = pElement->pContext;
    assert(pContext != NULL);

    drgui_element* pTopLevelElement = drgui_find_top_level_element(pElement);
    assert(pTopLevelElement != NULL);

    // The element needs to be added to the list of dirty elements if it doesn't exist already.
    bool isAlreadyDirty = false;
    for (size_t iDirtyElementCount = 0; iDirtyElementCount < pContext->dirtyElementCount; ++iDirtyElementCount) {
        if (pContext->ppDirtyElements[iDirtyElementCount] == pTopLevelElement) {
            isAlreadyDirty = true;
            break;
        }
    }

    if (!isAlreadyDirty) {
        if (pContext->dirtyElementCount == pContext->dirtyElementBufferSize) {
            size_t newBufferSize = pContext->dirtyElementBufferSize == 0 ? 1 : pContext->dirtyElementBufferSize*2;
            drgui_element** ppNewDirtyElements = (drgui_element**)realloc(pContext->ppDirtyElements, newBufferSize * sizeof(*ppNewDirtyElements));
            if (ppNewDirtyElements == NULL) {
                return NULL;
            }

            pContext->ppDirtyElements = ppNewDirtyElements;
            pContext->dirtyElementBufferSize = newBufferSize;
        }

        pContext->ppDirtyElements[pContext->dirtyCounter] = pTopLevelElement;
        pContext->dirtyElementCount += 1;
    }


    pContext->dirtyCounter += 1;
    return pTopLevelElement;
}

void drgui_end_dirty(drgui_element* pElement)
{
    if (pElement == NULL) {
        return;
    }

    drgui_context* pContext = pElement->pContext;
    assert(pContext != NULL);

    assert(pContext->dirtyElementCount > 0);
    assert(pContext->dirtyCounter > 0);

    pContext->dirtyCounter -= 1;
    if (pContext->dirtyCounter == 0)
    {
        for (size_t i = 0; i < pContext->dirtyElementCount; ++i) {
            drgui_post_outbound_event_dirty_global(pContext->ppDirtyElements[i], pContext->ppDirtyElements[i]->dirtyRect);
            pContext->ppDirtyElements[i]->dirtyRect = drgui_make_inside_out_rect();
        }

        pContext->dirtyElementCount = 0;
    }
}

void drgui_dirty(drgui_element* pElement, drgui_rect relativeRect)
{
    if (pElement == NULL) {
        return;
    }

    //drgui_context* pContext = pElement->pContext;
    //assert(pContext != NULL);

    drgui_element* pTopLevelElement = drgui_begin_dirty(pElement);
    if (pTopLevelElement == NULL) {
        return;
    }
    
    pTopLevelElement->dirtyRect = drgui_rect_union(pTopLevelElement->dirtyRect, drgui_make_rect_absolute(pElement, &relativeRect));
    drgui_end_dirty(pElement);
}


bool drgui_draw_iteration_callback(drgui_element* pElement, drgui_rect* pRelativeRect, void* pUserData)
{
    assert(pElement      != NULL);
    assert(pRelativeRect != NULL);

    if (pElement->onPaint != NULL)
    {
        // We want to set the initial clipping rectangle before drawing.
        drgui_set_clip(pElement, *pRelativeRect, pUserData);

        // We now call the painting function, but only after setting the clipping rectangle.
        pElement->onPaint(pElement, *pRelativeRect, pUserData);

        // The on_paint event handler may have adjusted the clipping rectangle so we need to ensure it's restored.
        drgui_set_clip(pElement, *pRelativeRect, pUserData);
    }

    return true;
}

void drgui_draw(drgui_element* pElement, drgui_rect relativeRect, void* pPaintData)
{
    if (pElement == NULL) {
        return;
    }

    drgui_context* pContext = pElement->pContext;
    if (pContext == NULL) {
        return;
    }

    assert(pContext->paintingCallbacks.drawBegin != NULL);
    assert(pContext->paintingCallbacks.drawEnd   != NULL);

    pContext->paintingCallbacks.drawBegin(pPaintData);
    {
        drgui_iterate_visible_elements(pElement, relativeRect, drgui_draw_iteration_callback, pPaintData);
    }
    pContext->paintingCallbacks.drawEnd(pPaintData);
}

void drgui_get_clip(drgui_element* pElement, drgui_rect* pRelativeRect, void* pPaintData)
{
    if (pElement == NULL || pElement->pContext == NULL) {
        return;
    }

    pElement->pContext->paintingCallbacks.getClip(pRelativeRect, pPaintData);

    // The clip returned by the drawing callback will be absolute so we'll need to convert that to relative.
    drgui_make_rect_relative(pElement, pRelativeRect);
}

void drgui_set_clip(drgui_element* pElement, drgui_rect relativeRect, void* pPaintData)
{
    if (pElement == NULL || pElement->pContext == NULL) {
        return;
    }


    // Make sure the rectangle is not negative.
    if (relativeRect.right < relativeRect.left) {
        relativeRect.right = relativeRect.left;
    }

    if (relativeRect.bottom < relativeRect.top) {
        relativeRect.bottom = relativeRect.top;
    }

    drgui_rect absoluteRect = relativeRect;
    drgui_make_rect_absolute(pElement, &absoluteRect);

    pElement->pContext->paintingCallbacks.setClip(absoluteRect, pPaintData);
}

void drgui_draw_rect(drgui_element* pElement, drgui_rect relativeRect, drgui_color color, void* pPaintData)
{
    if (pElement == NULL) {
        return;
    }

    assert(pElement->pContext != NULL);

    drgui_rect absoluteRect = relativeRect;
    drgui_make_rect_absolute(pElement, &absoluteRect);

    pElement->pContext->paintingCallbacks.drawRect(absoluteRect, color, pPaintData);
}

void drgui_draw_rect_outline(drgui_element* pElement, drgui_rect relativeRect, drgui_color color, float outlineWidth, void* pPaintData)
{
    if (pElement == NULL) {
        return;
    }

    assert(pElement->pContext != NULL);

    drgui_rect absoluteRect = relativeRect;
    drgui_make_rect_absolute(pElement, &absoluteRect);

    pElement->pContext->paintingCallbacks.drawRectOutline(absoluteRect, color, outlineWidth, pPaintData);
}

void drgui_draw_rect_with_outline(drgui_element * pElement, drgui_rect relativeRect, drgui_color color, float outlineWidth, drgui_color outlineColor, void * pPaintData)
{
    if (pElement == NULL) {
        return;
    }

    assert(pElement->pContext != NULL);

    drgui_rect absoluteRect = relativeRect;
    drgui_make_rect_absolute(pElement, &absoluteRect);

    pElement->pContext->paintingCallbacks.drawRectWithOutline(absoluteRect, color, outlineWidth, outlineColor, pPaintData);
}

void drgui_draw_round_rect(drgui_element* pElement, drgui_rect relativeRect, drgui_color color, float radius, void* pPaintData)
{
    if (pElement == NULL) {
        return;
    }

    assert(pElement->pContext != NULL);

    drgui_rect absoluteRect = relativeRect;
    drgui_make_rect_absolute(pElement, &absoluteRect);

    pElement->pContext->paintingCallbacks.drawRoundRect(absoluteRect, color, radius, pPaintData);
}

void drgui_draw_round_rect_outline(drgui_element* pElement, drgui_rect relativeRect, drgui_color color, float radius, float outlineWidth, void* pPaintData)
{
    if (pElement == NULL) {
        return;
    }

    assert(pElement->pContext != NULL);

    drgui_rect absoluteRect = relativeRect;
    drgui_make_rect_absolute(pElement, &absoluteRect);

    pElement->pContext->paintingCallbacks.drawRoundRectOutline(absoluteRect, color, radius, outlineWidth, pPaintData);
}

void drgui_draw_round_rect_with_outline(drgui_element* pElement, drgui_rect relativeRect, drgui_color color, float radius, float outlineWidth, drgui_color outlineColor, void* pPaintData)
{
    if (pElement == NULL) {
        return;
    }

    assert(pElement->pContext != NULL);

    drgui_rect absoluteRect = relativeRect;
    drgui_make_rect_absolute(pElement, &absoluteRect);

    pElement->pContext->paintingCallbacks.drawRoundRectWithOutline(absoluteRect, color, radius, outlineWidth, outlineColor, pPaintData);
}

void drgui_draw_text(drgui_element* pElement, drgui_font* pFont, const char* text, int textLengthInBytes, float posX, float posY, drgui_color color, drgui_color backgroundColor, void* pPaintData)
{
    if (pElement == NULL || pFont == NULL) {
        return;
    }

    assert(pElement->pContext != NULL);

    float absolutePosX = posX;
    float absolutePosY = posY;
    drgui_make_point_absolute(pElement, &absolutePosX, &absolutePosY);

    pElement->pContext->paintingCallbacks.drawText(pFont->internalFont, text, textLengthInBytes, absolutePosX, absolutePosY, color, backgroundColor, pPaintData);
}

void drgui_draw_image(drgui_element* pElement, drgui_image* pImage, drgui_draw_image_args* pArgs, void* pPaintData)
{
    if (pElement == NULL || pImage == NULL || pArgs == NULL) {
        return;
    }

    assert(pElement->pContext != NULL);

    drgui_make_point_absolute(pElement, &pArgs->dstX, &pArgs->dstY);
    drgui_make_point_absolute(pElement, &pArgs->dstBoundsX, &pArgs->dstBoundsY);

    if ((pArgs->options & DRGUI_IMAGE_ALIGN_CENTER) != 0)
    {
        pArgs->dstX = pArgs->dstBoundsX + (pArgs->dstBoundsWidth  - pArgs->dstWidth)  / 2;
        pArgs->dstY = pArgs->dstBoundsY + (pArgs->dstBoundsHeight - pArgs->dstHeight) / 2;
    }

    drgui_rect prevClip;
    pElement->pContext->paintingCallbacks.getClip(&prevClip, pPaintData);

    bool restoreClip = false;
    if ((pArgs->options & DRGUI_IMAGE_CLIP_BOUNDS) != 0)
    {
        // We only need to clip if part of the destination rectangle falls outside of the bounds.
        if (pArgs->dstX < pArgs->dstBoundsX || pArgs->dstX + pArgs->dstWidth  > pArgs->dstBoundsX + pArgs->dstBoundsWidth ||
            pArgs->dstY < pArgs->dstBoundsY || pArgs->dstY + pArgs->dstHeight > pArgs->dstBoundsY + pArgs->dstBoundsHeight)
        {
            restoreClip = true;
            pElement->pContext->paintingCallbacks.setClip(drgui_make_rect(pArgs->dstBoundsX, pArgs->dstBoundsY, pArgs->dstBoundsX + pArgs->dstBoundsWidth, pArgs->dstBoundsY + pArgs->dstBoundsHeight), pPaintData);
        }
    }

    if ((pArgs->options & DRGUI_IMAGE_DRAW_BOUNDS) != 0)
    {
        // The bounds is the area sitting around the outside of the destination rectangle.
        const float boundsLeft   = pArgs->dstBoundsX;
        const float boundsTop    = pArgs->dstBoundsY;
        const float boundsRight  = boundsLeft + pArgs->dstBoundsWidth;
        const float boundsBottom = boundsTop + pArgs->dstBoundsHeight;

        const float imageLeft   = pArgs->dstX;
        const float imageTop    = pArgs->dstY;
        const float imageRight  = imageLeft + pArgs->dstWidth;
        const float imageBottom = imageTop + pArgs->dstHeight;

        // Left.
        if (boundsLeft < imageLeft) {
            pElement->pContext->paintingCallbacks.drawRect(drgui_make_rect(boundsLeft, boundsTop, imageLeft, boundsBottom), pArgs->boundsColor, pPaintData);
        }

        // Right.
        if (boundsRight > imageRight) {
            pElement->pContext->paintingCallbacks.drawRect(drgui_make_rect(imageRight, boundsTop, boundsRight, boundsBottom), pArgs->boundsColor, pPaintData);
        }

        // Top.
        if (boundsTop < imageTop) {
            pElement->pContext->paintingCallbacks.drawRect(drgui_make_rect(imageLeft, boundsTop, imageRight, imageTop), pArgs->boundsColor, pPaintData);
        }

        // Bottom.
        if (boundsBottom > imageBottom) {
            pElement->pContext->paintingCallbacks.drawRect(drgui_make_rect(imageLeft, imageBottom, imageRight, boundsBottom), pArgs->boundsColor, pPaintData);
        }
    }

    pElement->pContext->paintingCallbacks.drawImage(pImage->hResource, pArgs, pPaintData);

    if (restoreClip) {
        pElement->pContext->paintingCallbacks.setClip(prevClip, pPaintData);
    }
}


drgui_font* drgui_create_font(drgui_context* pContext, const char* family, unsigned int size, drgui_font_weight weight, drgui_font_slant slant, float rotation, unsigned int flags)
{
    if (pContext == NULL) {
        return NULL;
    }

    if (pContext->paintingCallbacks.createFont == NULL) {
        return NULL;
    }


    drgui_resource internalFont = pContext->paintingCallbacks.createFont(pContext->pPaintingContext, family, size, weight, slant, rotation, flags);
    if (internalFont == NULL) {
        return NULL;
    }

    drgui_font* pFont = (drgui_font*)malloc(sizeof(drgui_font));
    if (pFont == NULL) {
        return NULL;
    }

    pFont->pContext     = pContext;
    pFont->family[0]    = '\0';
    pFont->size         = size;
    pFont->weight       = weight;
    pFont->slant        = slant;
    pFont->rotation     = rotation;
    pFont->flags        = flags;
    pFont->internalFont = internalFont;

    if (family != NULL) {
        drgui__strcpy_s(pFont->family, sizeof(pFont->family), family);
    }

    return pFont;
}

void drgui_delete_font(drgui_font* pFont)
{
    if (pFont == NULL) {
        return;
    }

    assert(pFont->pContext != NULL);

    // Delete the internal font objects first.
    if (pFont->pContext->paintingCallbacks.deleteFont) {
        pFont->pContext->paintingCallbacks.deleteFont(pFont->internalFont);
    }

    free(pFont);
}

bool drgui_get_font_metrics(drgui_font* pFont, drgui_font_metrics* pMetricsOut)
{
    if (pFont == NULL || pMetricsOut == NULL) {
        return false;
    }

    assert(pFont->pContext != NULL);

    if (pFont->pContext->paintingCallbacks.getFontMetrics == NULL) {
        return false;
    }

    return pFont->pContext->paintingCallbacks.getFontMetrics(pFont->internalFont, pMetricsOut);
}

bool drgui_get_glyph_metrics(drgui_font* pFont, unsigned int utf32, drgui_glyph_metrics* pMetricsOut)
{
    if (pFont == NULL || pMetricsOut == NULL) {
        return false;
    }

    assert(pFont->pContext != NULL);

    if (pFont->pContext->paintingCallbacks.getGlyphMetrics == NULL) {
        return false;
    }

    return pFont->pContext->paintingCallbacks.getGlyphMetrics(pFont->internalFont, utf32, pMetricsOut);
}

bool drgui_measure_string(drgui_font* pFont, const char* text, size_t textLengthInBytes, float* pWidthOut, float* pHeightOut)
{
    if (pFont == NULL) {
        return false;
    }

    if (text == NULL || textLengthInBytes == 0)
    {
        drgui_font_metrics metrics;
        if (!drgui_get_font_metrics(pFont, &metrics)) {
            return false;
        }

        if (pWidthOut) {
            *pWidthOut = 0;
        }
        if (pHeightOut) {
            *pHeightOut = (float)metrics.lineHeight;
        }

        return true;
    }



    assert(pFont->pContext != NULL);

    if (pFont->pContext->paintingCallbacks.measureString == NULL) {
        return false;
    }

    return pFont->pContext->paintingCallbacks.measureString(pFont->internalFont, text, textLengthInBytes, pWidthOut, pHeightOut);
}

bool drgui_get_text_cursor_position_from_point(drgui_font* pFont, const char* text, size_t textSizeInBytes, float maxWidth, float inputPosX, float* pTextCursorPosXOut, size_t* pCharacterIndexOut)
{
    if (pFont == NULL) {
        return false;
    }

    assert(pFont->pContext != NULL);

    if (pFont->pContext->paintingCallbacks.getTextCursorPositionFromPoint) {
        return pFont->pContext->paintingCallbacks.getTextCursorPositionFromPoint(pFont->internalFont, text, textSizeInBytes, maxWidth, inputPosX, pTextCursorPosXOut, pCharacterIndexOut);
    }

    return false;
}

bool drgui_get_text_cursor_position_from_char(drgui_font* pFont, const char* text, size_t characterIndex, float* pTextCursorPosXOut)
{
    if (pFont == NULL) {
        return false;
    }

    assert(pFont->pContext != NULL);

    if (pFont->pContext->paintingCallbacks.getTextCursorPositionFromChar) {
        return pFont->pContext->paintingCallbacks.getTextCursorPositionFromChar(pFont->internalFont, text, characterIndex, pTextCursorPosXOut);
    }

    return false;
}



drgui_image* drgui_create_image(drgui_context* pContext, unsigned int width, unsigned int height, drgui_image_format format, unsigned int stride, const void* pData)
{
    if (pContext == NULL) {
        return NULL;
    }

    if (pContext->paintingCallbacks.createImage == NULL) {
        return NULL;
    }


    // If the stride is 0, assume tightly packed.
    if (stride == 0) {
        stride = width * 4;
    }


    drgui_resource internalImage = pContext->paintingCallbacks.createImage(pContext->pPaintingContext, width, height, format, stride, pData);
    if (internalImage == NULL) {
        return NULL;
    }

    drgui_image* pImage = (drgui_image*)malloc(sizeof(*pImage));
    if (pImage == NULL) {
        return NULL;
    }

    pImage->pContext  = pContext;
    pImage->hResource = internalImage;


    return pImage;
}

void drgui_delete_image(drgui_image* pImage)
{
    if (pImage == NULL) {
        return;
    }

    assert(pImage->pContext != NULL);

    // Delete the internal font object.
    if (pImage->pContext->paintingCallbacks.deleteImage) {
        pImage->pContext->paintingCallbacks.deleteImage(pImage->hResource);
    }

    // Free the font object last.
    free(pImage);
}

void drgui_get_image_size(drgui_image* pImage, unsigned int* pWidthOut, unsigned int* pHeightOut)
{
    if (pWidthOut) *pWidthOut = 0;
    if (pHeightOut) *pHeightOut = 0;

    if (pImage == NULL) {
        return;
    }

    assert(pImage->pContext != NULL);

    if (pImage->pContext->paintingCallbacks.getImageSize == NULL) {
        return;
    }

    pImage->pContext->paintingCallbacks.getImageSize(pImage->hResource, pWidthOut, pHeightOut);
}

drgui_image_format drgui_get_optimal_image_format(drgui_context* pContext)
{
    if (pContext == NULL || pContext->paintingCallbacks.getOptimalImageFormat == NULL) {
        return drgui_image_format_rgba8;
    }

    return pContext->paintingCallbacks.getOptimalImageFormat(pContext->pPaintingContext);
}

void* drgui_map_image_data(drgui_image* pImage, unsigned int accessFlags)
{
    if (pImage == NULL) {
        return NULL;
    }

    if (pImage->pContext->paintingCallbacks.mapImageData == NULL || pImage->pContext->paintingCallbacks.unmapImageData == NULL) {
        return NULL;
    }

    return pImage->pContext->paintingCallbacks.mapImageData(pImage->hResource, accessFlags);
}

void drgui_unmap_image_data(drgui_image* pImage)
{
    if (pImage == NULL) {
        return;
    }

    if (pImage->pContext->paintingCallbacks.unmapImageData == NULL) {
        return;
    }

    pImage->pContext->paintingCallbacks.unmapImageData(pImage->hResource);
}



/////////////////////////////////////////////////////////////////
//
// HIGH-LEVEL API
//
/////////////////////////////////////////////////////////////////

//// Hit Testing and Layout ////

void drgui_on_size_fit_children_to_parent(drgui_element* pElement, float newWidth, float newHeight)
{
    for (drgui_element* pChild = pElement->pFirstChild; pChild != NULL; pChild = pChild->pNextSibling) {
        drgui_set_size(pChild, newWidth, newHeight);
    }
}

bool drgui_pass_through_hit_test(drgui_element* pElement, float mousePosX, float mousePosY)
{
    (void)pElement;
    (void)mousePosX;
    (void)mousePosY;

    return false;
}



//// Painting ////

void drgui_draw_border(drgui_element* pElement, float borderWidth, drgui_color color, void* pUserData)
{
    drgui_draw_rect_outline(pElement, drgui_get_local_rect(pElement), color, borderWidth, pUserData);
}



/////////////////////////////////////////////////////////////////
//
// UTILITY API
//
/////////////////////////////////////////////////////////////////

drgui_color drgui_rgba(drgui_byte r, drgui_byte g, drgui_byte b, drgui_byte a)
{
    drgui_color color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;

    return color;
}

drgui_color drgui_rgb(drgui_byte r, drgui_byte g, drgui_byte b)
{
    drgui_color color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = 255;

    return color;
}

drgui_rect drgui_clamp_rect(drgui_rect rect, drgui_rect other)
{
    drgui_rect result;
    result.left   = (rect.left   >= other.left)   ? rect.left   : other.left;
    result.top    = (rect.top    >= other.top)    ? rect.top    : other.top;
    result.right  = (rect.right  <= other.right)  ? rect.right  : other.right;
    result.bottom = (rect.bottom <= other.bottom) ? rect.bottom : other.bottom;

    return result;
}

bool drgui_clamp_rect_to_element(const drgui_element* pElement, drgui_rect* pRelativeRect)
{
    if (pElement == NULL || pRelativeRect == NULL) {
        return false;
    }


    if (pRelativeRect->left < 0) {
        pRelativeRect->left = 0;
    }
    if (pRelativeRect->top < 0) {
        pRelativeRect->top = 0;
    }

    if (pRelativeRect->right > pElement->width) {
        pRelativeRect->right = pElement->width;
    }
    if (pRelativeRect->bottom > pElement->height) {
        pRelativeRect->bottom = pElement->height;
    }


    return (pRelativeRect->right - pRelativeRect->left > 0) && (pRelativeRect->bottom - pRelativeRect->top > 0);
}

drgui_rect drgui_make_rect_relative(const drgui_element* pElement, drgui_rect* pRect)
{
    if (pElement == NULL || pRect == NULL) {
        return drgui_make_rect(0, 0, 0, 0);
    }

    pRect->left   -= pElement->absolutePosX;
    pRect->top    -= pElement->absolutePosY;
    pRect->right  -= pElement->absolutePosX;
    pRect->bottom -= pElement->absolutePosY;

    return *pRect;
}

drgui_rect drgui_make_rect_absolute(const drgui_element * pElement, drgui_rect * pRect)
{
    if (pElement == NULL || pRect == NULL) {
        return drgui_make_rect(0, 0, 0, 0);
    }

    pRect->left   += pElement->absolutePosX;
    pRect->top    += pElement->absolutePosY;
    pRect->right  += pElement->absolutePosX;
    pRect->bottom += pElement->absolutePosY;

    return *pRect;
}

void drgui_make_point_relative(const drgui_element* pElement, float* positionX, float* positionY)
{
    if (pElement != NULL)
    {
        if (positionX != NULL) {
            *positionX -= pElement->absolutePosX;
        }

        if (positionY != NULL) {
            *positionY -= pElement->absolutePosY;
        }
    }
}

void drgui_make_point_absolute(const drgui_element* pElement, float* positionX, float* positionY)
{
    if (pElement != NULL)
    {
        if (positionX != NULL) {
            *positionX += pElement->absolutePosX;
        }

        if (positionY != NULL) {
            *positionY += pElement->absolutePosY;
        }
    }
}

drgui_rect drgui_make_rect(float left, float top, float right, float bottom)
{
    drgui_rect rect;
    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;

    return rect;
}

drgui_rect drgui_make_inside_out_rect()
{
    drgui_rect rect;
    rect.left   =  FLT_MAX;
    rect.top    =  FLT_MAX;
    rect.right  = -FLT_MAX;
    rect.bottom = -FLT_MAX;

    return rect;
}

drgui_rect drgui_grow_rect(drgui_rect rect, float amount)
{
    drgui_rect result = rect;
    result.left   -= amount;
    result.top    -= amount;
    result.right  += amount;
    result.bottom += amount;

    return result;
}

drgui_rect drgui_scale_rect(drgui_rect rect, float scaleX, float scaleY)
{
    drgui_rect result = rect;
    result.left   *= scaleX;
    result.top    *= scaleY;
    result.right  *= scaleX;
    result.bottom *= scaleY;

    return result;
}

drgui_rect drgui_offset_rect(drgui_rect rect, float offsetX, float offsetY)
{
    return drgui_make_rect(rect.left + offsetX, rect.top + offsetY, rect.right + offsetX, rect.bottom + offsetY);
}

drgui_rect drgui_rect_union(drgui_rect rect0, drgui_rect rect1)
{
    drgui_rect result;
    result.left   = (rect0.left   < rect1.left)   ? rect0.left   : rect1.left;
    result.top    = (rect0.top    < rect1.top)    ? rect0.top    : rect1.top;
    result.right  = (rect0.right  > rect1.right)  ? rect0.right  : rect1.right;
    result.bottom = (rect0.bottom > rect1.bottom) ? rect0.bottom : rect1.bottom;

    return result;
}

bool drgui_rect_contains_point(drgui_rect rect, float posX, float posY)
{
    if (posX < rect.left || posY < rect.top) {
        return false;
    }

    if (posX >= rect.right || posY >= rect.bottom) {
        return false;
    }

    return true;
}

bool drgui_rect_equal(drgui_rect rect0, drgui_rect rect1)
{
    return
        rect0.left   == rect1.left  &&
        rect0.top    == rect1.top   &&
        rect0.right  == rect1.right &&
        rect0.bottom == rect1.bottom;
}

bool drgui_rect_has_volume(drgui_rect rect)
{
    return rect.right > rect.left && rect.bottom > rect.top;
}




/////////////////////////////////////////////////////////////////
//
// EASY_DRAW-SPECIFIC API
//
/////////////////////////////////////////////////////////////////
#ifndef DRGUI_NO_DR_2D

void drgui_draw_begin_dr_2d(void* pPaintData);
void drgui_draw_end_dr_2d(void* pPaintData);
void drgui_set_clip_dr_2d(drgui_rect rect, void* pPaintData);
void drgui_get_clip_dr_2d(drgui_rect* pRectOut, void* pPaintData);
void drgui_draw_rect_dr_2d(drgui_rect rect, drgui_color color, void* pPaintData);
void drgui_draw_rect_outline_dr_2d(drgui_rect, drgui_color, float, void*);
void drgui_draw_rect_with_outline_dr_2d(drgui_rect, drgui_color, float, drgui_color, void*);
void drgui_draw_round_rect_dr_2d(drgui_rect, drgui_color, float, void*);
void drgui_draw_round_rect_outline_dr_2d(drgui_rect, drgui_color, float, float, void*);
void drgui_draw_round_rect_with_outline_dr_2d(drgui_rect, drgui_color, float, float, drgui_color, void*);
void drgui_draw_text_dr_2d(drgui_resource, const char*, int, float, float, drgui_color, drgui_color, void*);
void drgui_draw_image_dr_2d(drgui_resource image, drgui_draw_image_args* pArgs, void* pPaintData);

drgui_resource drgui_create_font_dr_2d(void*, const char*, unsigned int, drgui_font_weight, drgui_font_slant, float, unsigned int flags);
void drgui_delete_font_dr_2d(drgui_resource);
unsigned int drgui_get_font_size_dr_2d(drgui_resource hFont);
bool drgui_get_font_metrics_dr_2d(drgui_resource, drgui_font_metrics*);
bool drgui_get_glyph_metrics_dr_2d(drgui_resource, unsigned int, drgui_glyph_metrics*);
bool drgui_measure_string_dr_2d(drgui_resource, const char*, size_t, float*, float*);
bool drgui_get_text_cursor_position_from_point_dr_2d(drgui_resource font, const char* text, size_t textSizeInBytes, float maxWidth, float inputPosX, float* pTextCursorPosXOut, size_t* pCharacterIndexOut);
bool drgui_get_text_cursor_position_from_char_dr_2d(drgui_resource font, const char* text, size_t characterIndex, float* pTextCursorPosXOut);

drgui_resource drgui_create_image_dr_2d(void* pPaintingContext, unsigned int width, unsigned int height, drgui_image_format format, unsigned int stride, const void* pImageData);
void drgui_delete_image_dr_2d(drgui_resource image);
void drgui_get_image_size_dr_2d(drgui_resource image, unsigned int* pWidthOut, unsigned int* pHeightOut);
drgui_image_format drgui_get_optimal_image_format_dr_2d(void* pPaintingContext);
void* drgui_map_image_data_dr_2d(drgui_resource image, unsigned int accessFlags);
void drgui_unmap_image_data_dr_2d(drgui_resource image);

drgui_context* drgui_create_context_dr_2d(dr2d_context* pDrawingContext)
{
    drgui_context* pContext = drgui_create_context();
    if (pContext != NULL) {
        drgui_register_dr_2d_callbacks(pContext, pDrawingContext);
    }

    return pContext;
}

void drgui_register_dr_2d_callbacks(drgui_context* pContext, dr2d_context* pDrawingContext)
{
    drgui_painting_callbacks callbacks;
    callbacks.drawBegin                      = drgui_draw_begin_dr_2d;
    callbacks.drawEnd                        = drgui_draw_end_dr_2d;
    callbacks.setClip                        = drgui_set_clip_dr_2d;
    callbacks.getClip                        = drgui_get_clip_dr_2d;
    callbacks.drawRect                       = drgui_draw_rect_dr_2d;
    callbacks.drawRectOutline                = drgui_draw_rect_outline_dr_2d;
    callbacks.drawRectWithOutline            = drgui_draw_rect_with_outline_dr_2d;
    callbacks.drawRoundRect                  = drgui_draw_round_rect_dr_2d;
    callbacks.drawRoundRectOutline           = drgui_draw_round_rect_outline_dr_2d;
    callbacks.drawRoundRectWithOutline       = drgui_draw_round_rect_with_outline_dr_2d;
    callbacks.drawText                       = drgui_draw_text_dr_2d;
    callbacks.drawImage                      = drgui_draw_image_dr_2d;

    callbacks.createFont                     = drgui_create_font_dr_2d;
    callbacks.deleteFont                     = drgui_delete_font_dr_2d;
    callbacks.getFontSize                    = drgui_get_font_size_dr_2d;
    callbacks.getFontMetrics                 = drgui_get_font_metrics_dr_2d;
    callbacks.getGlyphMetrics                = drgui_get_glyph_metrics_dr_2d;
    callbacks.measureString                  = drgui_measure_string_dr_2d;

    callbacks.createImage                    = drgui_create_image_dr_2d;
    callbacks.deleteImage                    = drgui_delete_image_dr_2d;
    callbacks.getImageSize                   = drgui_get_image_size_dr_2d;
    callbacks.getOptimalImageFormat          = drgui_get_optimal_image_format_dr_2d;
    callbacks.mapImageData                   = drgui_map_image_data_dr_2d;
    callbacks.unmapImageData                 = drgui_unmap_image_data_dr_2d;

    callbacks.getTextCursorPositionFromPoint = drgui_get_text_cursor_position_from_point_dr_2d;
    callbacks.getTextCursorPositionFromChar  = drgui_get_text_cursor_position_from_char_dr_2d;

    drgui_register_painting_callbacks(pContext, pDrawingContext, callbacks);
}


void drgui_draw_begin_dr_2d(void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_begin_draw(pSurface);
}

void drgui_draw_end_dr_2d(void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_end_draw(pSurface);
}

void drgui_set_clip_dr_2d(drgui_rect rect, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_set_clip(pSurface, rect.left, rect.top, rect.right, rect.bottom);
}

void drgui_get_clip_dr_2d(drgui_rect* pRectOut, void* pPaintData)
{
    assert(pRectOut != NULL);

    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_get_clip(pSurface, &pRectOut->left, &pRectOut->top, &pRectOut->right, &pRectOut->bottom);
}

void drgui_draw_rect_dr_2d(drgui_rect rect, drgui_color color, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_draw_rect(pSurface, rect.left, rect.top, rect.right, rect.bottom, dr2d_rgba(color.r, color.g, color.b, color.a));
}

void drgui_draw_rect_outline_dr_2d(drgui_rect rect, drgui_color color, float outlineWidth, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_draw_rect_outline(pSurface, rect.left, rect.top, rect.right, rect.bottom, dr2d_rgba(color.r, color.g, color.b, color.a), outlineWidth);
}

void drgui_draw_rect_with_outline_dr_2d(drgui_rect rect, drgui_color color, float outlineWidth, drgui_color outlineColor, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_draw_rect_with_outline(pSurface, rect.left, rect.top, rect.right, rect.bottom, dr2d_rgba(color.r, color.g, color.b, color.a), outlineWidth, dr2d_rgba(outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a));
}

void drgui_draw_round_rect_dr_2d(drgui_rect rect, drgui_color color, float radius, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_draw_round_rect(pSurface, rect.left, rect.top, rect.right, rect.bottom, dr2d_rgba(color.r, color.g, color.b, color.a), radius);
}

void drgui_draw_round_rect_outline_dr_2d(drgui_rect rect, drgui_color color, float radius, float outlineWidth, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_draw_round_rect_outline(pSurface, rect.left, rect.top, rect.right, rect.bottom, dr2d_rgba(color.r, color.g, color.b, color.a), radius, outlineWidth);
}

void drgui_draw_round_rect_with_outline_dr_2d(drgui_rect rect, drgui_color color, float radius, float outlineWidth, drgui_color outlineColor, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_draw_round_rect_with_outline(pSurface, rect.left, rect.top, rect.right, rect.bottom, dr2d_rgba(color.r, color.g, color.b, color.a), radius, outlineWidth, dr2d_rgba(outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a));
}

void drgui_draw_text_dr_2d(drgui_resource font, const char* text, int textSizeInBytes, float posX, float posY, drgui_color color, drgui_color backgroundColor, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_draw_text(pSurface, (dr2d_font*)font, text, textSizeInBytes, posX, posY, dr2d_rgba(color.r, color.g, color.b, color.a), dr2d_rgba(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a));
}

void drgui_draw_image_dr_2d(drgui_resource image, drgui_draw_image_args* pArgs, void* pPaintData)
{
    dr2d_surface* pSurface = (dr2d_surface*)pPaintData;
    assert(pSurface != NULL);

    dr2d_draw_image_args args;
    args.dstX            = pArgs->dstX;
    args.dstY            = pArgs->dstY;
    args.dstWidth        = pArgs->dstWidth;
    args.dstHeight       = pArgs->dstHeight;
    args.srcX            = pArgs->srcX;
    args.srcY            = pArgs->srcY;
    args.srcWidth        = pArgs->srcWidth;
    args.srcHeight       = pArgs->srcHeight;
    args.foregroundTint  = dr2d_rgba(pArgs->foregroundTint.r, pArgs->foregroundTint.g, pArgs->foregroundTint.b, pArgs->foregroundTint.a);
    args.backgroundColor = dr2d_rgba(pArgs->backgroundColor.r, pArgs->backgroundColor.g, pArgs->backgroundColor.b, pArgs->backgroundColor.a);
    args.options         = pArgs->options;
    dr2d_draw_image(pSurface, (dr2d_image*)image, &args);
}


drgui_resource drgui_create_font_dr_2d(void* pPaintingContext, const char* family, unsigned int size, drgui_font_weight weight, drgui_font_slant slant, float rotation, unsigned int flags)
{
    return dr2d_create_font((dr2d_context*)pPaintingContext, family, size, (dr2d_font_weight)weight, (dr2d_font_slant)slant, rotation, flags);
}

void drgui_delete_font_dr_2d(drgui_resource font)
{
    dr2d_delete_font((dr2d_font*)font);
}

unsigned int drgui_get_font_size_dr_2d(drgui_resource font)
{
    return dr2d_get_font_size((dr2d_font*)font);
}

bool drgui_get_font_metrics_dr_2d(drgui_resource font, drgui_font_metrics* pMetricsOut)
{
    assert(pMetricsOut != NULL);

    dr2d_font_metrics metrics;
    if (!dr2d_get_font_metrics((dr2d_font*)font, &metrics)) {
        return false;
    }

    pMetricsOut->ascent     = metrics.ascent;
    pMetricsOut->descent    = metrics.descent;
    pMetricsOut->lineHeight = metrics.lineHeight;
    pMetricsOut->spaceWidth = metrics.spaceWidth;

    return true;
}

bool drgui_get_glyph_metrics_dr_2d(drgui_resource font, unsigned int utf32, drgui_glyph_metrics* pMetricsOut)
{
    assert(pMetricsOut != NULL);

    dr2d_glyph_metrics metrics;
    if (!dr2d_get_glyph_metrics((dr2d_font*)font, utf32, &metrics)) {
        return false;
    }

    pMetricsOut->width    = metrics.width;
    pMetricsOut->height   = metrics.height;
    pMetricsOut->originX  = metrics.originX;
    pMetricsOut->originY  = metrics.originY;
    pMetricsOut->advanceX = metrics.advanceX;
    pMetricsOut->advanceY = metrics.advanceY;

    return true;
}

bool drgui_measure_string_dr_2d(drgui_resource font, const char* text, size_t textSizeInBytes, float* pWidthOut, float* pHeightOut)
{
    return dr2d_measure_string((dr2d_font*)font, text, textSizeInBytes, pWidthOut, pHeightOut);
}

bool drgui_get_text_cursor_position_from_point_dr_2d(drgui_resource font, const char* text, size_t textSizeInBytes, float maxWidth, float inputPosX, float* pTextCursorPosXOut, size_t* pCharacterIndexOut)
{
    return dr2d_get_text_cursor_position_from_point((dr2d_font*)font, text, textSizeInBytes, maxWidth, inputPosX, pTextCursorPosXOut, pCharacterIndexOut);
}

bool drgui_get_text_cursor_position_from_char_dr_2d(drgui_resource font, const char* text, size_t characterIndex, float* pTextCursorPosXOut)
{
    return dr2d_get_text_cursor_position_from_char((dr2d_font*)font, text, characterIndex, pTextCursorPosXOut);
}


drgui_resource drgui_create_image_dr_2d(void* pPaintingContext, unsigned int width, unsigned int height, drgui_image_format format, unsigned int stride, const void* pImageData)
{
    dr2d_image_format dr2dFormat;
    switch (format)
    {
    case drgui_image_format_bgra8: dr2dFormat = dr2d_image_format_bgra8; break;
    case drgui_image_format_argb8: dr2dFormat = dr2d_image_format_argb8; break;
    default: dr2dFormat = dr2d_image_format_rgba8;
    }

    return dr2d_create_image((dr2d_context*)pPaintingContext, width, height, dr2dFormat, stride, pImageData);
}

void drgui_delete_image_dr_2d(drgui_resource image)
{
    dr2d_delete_image((dr2d_image*)image);
}

void drgui_get_image_size_dr_2d(drgui_resource image, unsigned int* pWidthOut, unsigned int* pHeightOut)
{
    dr2d_get_image_size((dr2d_image*)image, pWidthOut, pHeightOut);
}

drgui_image_format drgui_get_optimal_image_format_dr_2d(void* pPaintingContext)
{
    return (drgui_image_format)dr2d_get_optimal_image_format((dr2d_context*)pPaintingContext);
}

void* drgui_map_image_data_dr_2d(drgui_resource image, unsigned int accessFlags)
{
    return dr2d_map_image_data((dr2d_image*)image, accessFlags);
}

void drgui_unmap_image_data_dr_2d(drgui_resource image)
{
    dr2d_unmap_image_data((dr2d_image*)image);
}

#endif  //DRGUI_NO_DR_2D