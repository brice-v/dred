// Copyright (C) 2018 David Reid. See included LICENSE file.

dtk_result dtk_tabbar_tab_init(dtk_tabbar* pTabBar, const char* text, dtk_control* pTabPage, dtk_tabbar_tab* pTab)
{
    (void)pTabBar;  // Not used for now, but will probably be used later when more efficient memory management is implemented.

    if (pTabBar == NULL) return DTK_INVALID_ARGS;
    dtk_zero_object(pTab);

    pTab->pText = dtk_make_string(text);
    pTab->pPage = pTabPage;

    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_tab_uninit(dtk_tabbar_tab* pTab)
{
    if (pTab == NULL) return DTK_INVALID_ARGS;
    
    dtk_free_string(pTab->pText);
    dtk_free_string(pTab->pTooltipText);
    return DTK_SUCCESS;
}


typedef struct
{
    dtk_int32 posX;
    dtk_int32 posY;
    dtk_uint32 width;
    dtk_uint32 height;
    dtk_rect textRect;
    dtk_rect closeButtonRect;
    dtk_rect pinButtonRect;
    dtk_tabbar_tab* pTab;
    dtk_bool32 _isLast;             // Internal use only. Used to indicate whether or not this iterator represents the last tab.
    dtk_int32 _nextIndex;           // Internal use only.
    dtk_int32 _longestTextWidth;    // Internal use only.
} dtk_tabbar__iterator;

dtk_int32 dtk_tabbar__find_longest_tab_text(const dtk_tabbar* pTabBar)
{
    float uiScale = dtk_control_get_scaling_factor(DTK_CONTROL(pTabBar));
    dtk_int32 longestWidth = 0;

    for (dtk_uint32 iTab = 0; iTab < pTabBar->tabCount; ++iTab) {
        dtk_tabbar_tab* pTab = &pTabBar->pTabs[iTab];

        dtk_int32 textWidth;
        dtk_int32 textHeight;
        dtk_font_measure_string(dtk_tabbar_get_font(pTabBar), uiScale, pTab->pText, strlen(pTab->pText), &textWidth, &textHeight);

        if (longestWidth < textWidth) {
            longestWidth = textWidth;
        }
    }

    return longestWidth;
}

dtk_bool32 dtk_tabbar__next_tab(const dtk_tabbar* pTabBar, dtk_tabbar__iterator* pIterator)
{
    dtk_assert(pTabBar != NULL);
    dtk_assert(pIterator != NULL);

    if (pIterator->_isLast) {
        return DTK_FALSE;   // Reached the end of iteration.
    }

    float uiScale = dtk_control_get_scaling_factor(DTK_CONTROL(pTabBar));

    pIterator->_nextIndex += 1;
    if ((dtk_uint32)pIterator->_nextIndex+1 == pTabBar->tabCount) {
        pIterator->_isLast = DTK_TRUE;
    }

    pIterator->pTab = &pTabBar->pTabs[pIterator->_nextIndex];


    // The close button also affects the size of each tab.
    dtk_uint32 closeButtonImageWidth = 0;
    dtk_uint32 closeButtonImageHeight = 0;
    dtk_uint32 closeButtonImageWidthWithPadding = 0;
    dtk_uint32 closeButtonImageHeightWithPadding = 0;
    if (dtk_tabbar_is_showing_close_button(pTabBar)) {
        dtk_tabbar_get_close_button_size(pTabBar, &closeButtonImageWidth, &closeButtonImageHeight);

        closeButtonImageWidth  = (dtk_uint32)(closeButtonImageWidth  * uiScale);
        closeButtonImageHeight = (dtk_uint32)(closeButtonImageHeight * uiScale);

        closeButtonImageWidthWithPadding  = closeButtonImageWidth  + (dtk_uint32)((pTabBar->closeButtonPaddingLeft + pTabBar->closeButtonPaddingRight ) * uiScale);
        closeButtonImageHeightWithPadding = closeButtonImageHeight + (dtk_uint32)((pTabBar->closeButtonPaddingTop  + pTabBar->closeButtonPaddingBottom) * uiScale);
    }


    dtk_uint32 prevTabWidth  = pIterator->width;
    dtk_uint32 prevTabHeight = pIterator->height;

    // The size and position of each tab depends on the flow and text direction of the tabbar.
    dtk_int32 actualTextWidth;
    dtk_int32 actualTextHeight;
    if (pIterator->pTab->pText != NULL) {
        dtk_font_measure_string(dtk_tabbar_get_font(pTabBar), uiScale, pIterator->pTab->pText, strlen(pIterator->pTab->pText), &actualTextWidth, &actualTextHeight);
    } else {
        actualTextWidth = 0;
        actualTextHeight = 0;

        dtk_font_metrics metrics;
        dtk_result result = dtk_font_get_metrics(dtk_tabbar_get_font(pTabBar), uiScale, &metrics);
        if (result == DTK_SUCCESS) {
            actualTextHeight = metrics.lineHeight;
        }
    }

    

    dtk_int32 longestTextWidth  = actualTextWidth;
    dtk_int32 longestTextHeight = actualTextHeight;
    if ((pTabBar->textDirection == dtk_tabbar_text_direction_horizontal && (pTabBar->flow == dtk_tabbar_flow_top_to_bottom || pTabBar->flow == dtk_tabbar_flow_bottom_to_top)) ||
        (pTabBar->textDirection == dtk_tabbar_text_direction_vertical   && (pTabBar->flow == dtk_tabbar_flow_left_to_right || pTabBar->flow == dtk_tabbar_flow_right_to_left))) {
        longestTextWidth = pIterator->_longestTextWidth;
    }

    // These are set in the switch below.
    dtk_int32 orientedTextWidth = 0;
    dtk_int32 orientedTextHeight = 0;
    dtk_uint32 orientedCloseButtonWidth = 0;
    dtk_uint32 orientedCloseButtonHeight = 0;


    dtk_uint32 nextTabWidth = 0;
    dtk_uint32 nextTabHeight = 0;
    switch (pTabBar->textDirection)
    {
        case dtk_tabbar_text_direction_horizontal:
        {
            orientedTextWidth = actualTextWidth;
            orientedTextHeight = actualTextHeight;
            orientedCloseButtonWidth = closeButtonImageWidthWithPadding;
            orientedCloseButtonHeight = closeButtonImageHeightWithPadding;
            nextTabWidth  = (dtk_uint32)longestTextWidth + orientedCloseButtonWidth;
            nextTabHeight = dtk_max((dtk_uint32)longestTextHeight, orientedCloseButtonHeight);
        } break;
        case dtk_tabbar_text_direction_vertical:
        {
            orientedTextWidth = actualTextHeight;
            orientedTextHeight = actualTextWidth;
            orientedCloseButtonWidth = closeButtonImageHeightWithPadding;
            orientedCloseButtonHeight = closeButtonImageWidthWithPadding;
            nextTabWidth  = dtk_max((dtk_uint32)longestTextHeight, orientedCloseButtonWidth);
            nextTabHeight = (dtk_uint32)longestTextWidth + orientedCloseButtonHeight;
        } break;
        default: break; // Will never hit this.
    }

    dtk_uint32 paddingLeftScaled   = (dtk_uint32)(pTabBar->paddingLeft   * uiScale);
    dtk_uint32 paddingTopScaled    = (dtk_uint32)(pTabBar->paddingTop    * uiScale);
    dtk_uint32 paddingRightScaled  = (dtk_uint32)(pTabBar->paddingRight  * uiScale);
    dtk_uint32 paddingBottomScaled = (dtk_uint32)(pTabBar->paddingBottom * uiScale);


    nextTabWidth  += paddingLeftScaled + paddingRightScaled;
    nextTabHeight += paddingTopScaled + paddingBottomScaled;

    switch (pTabBar->flow)
    {
        case dtk_tabbar_flow_left_to_right:
        {
            pIterator->posX = pIterator->posX + prevTabWidth;
        } break;
        case dtk_tabbar_flow_top_to_bottom:
        {
            pIterator->posY = pIterator->posY + prevTabHeight;
        } break;
        case dtk_tabbar_flow_right_to_left:
        {
            pIterator->posX = pIterator->posX - nextTabWidth;
        } break;
        case dtk_tabbar_flow_bottom_to_top:
        {
            pIterator->posY = pIterator->posY - nextTabHeight;
        } break;
        default: break; // Will never hit this.
    }


    // At this point we have all the information we need to calculate the positions for each of the different elements.
    switch (pTabBar->textDirection)
    {
        case dtk_tabbar_text_direction_horizontal:
        {
            pIterator->textRect.left   = paddingLeftScaled;
            pIterator->textRect.top    = paddingTopScaled + (((dtk_int32)(nextTabHeight - paddingBottomScaled - paddingTopScaled) - (dtk_int32)orientedTextHeight)/2);
            pIterator->textRect.right  = pIterator->textRect.left + orientedTextWidth;
            pIterator->textRect.bottom = pIterator->textRect.top + orientedTextHeight;

            if (dtk_tabbar_is_showing_close_button(pTabBar)) {
                pIterator->closeButtonRect.left   = paddingLeftScaled + longestTextWidth + (dtk_uint32)(pTabBar->closeButtonPaddingLeft * uiScale);
                pIterator->closeButtonRect.top    = paddingTopScaled + (((dtk_int32)(nextTabHeight - paddingBottomScaled - paddingTopScaled) - (dtk_int32)closeButtonImageHeight)/2);
                pIterator->closeButtonRect.right  = pIterator->closeButtonRect.left + closeButtonImageWidth;
                pIterator->closeButtonRect.bottom = pIterator->closeButtonRect.top + closeButtonImageHeight;
            }
        } break;
        case dtk_tabbar_text_direction_vertical:
        {
            pIterator->textRect.left   = paddingLeftScaled + (((dtk_int32)(nextTabWidth - paddingRightScaled - paddingLeftScaled) - (dtk_int32)orientedTextWidth)/2);
            pIterator->textRect.top    = paddingTopScaled;
            pIterator->textRect.right  = pIterator->textRect.left + orientedTextWidth;
            pIterator->textRect.bottom = pIterator->textRect.top + orientedTextHeight;

            if (dtk_tabbar_is_showing_close_button(pTabBar)) {
                dtk_uint32 imageWidth;
                dtk_uint32 imageHeight;
                dtk_tabbar_get_close_button_size(pTabBar, &imageWidth, &imageHeight);

                pIterator->closeButtonRect.left   = paddingLeftScaled + (((dtk_int32)(nextTabWidth - paddingRightScaled - paddingLeftScaled) - (dtk_int32)closeButtonImageWidth)/2);
                pIterator->closeButtonRect.top    = paddingTopScaled + longestTextWidth + (dtk_uint32)(pTabBar->closeButtonPaddingLeft * uiScale);
                pIterator->closeButtonRect.right  = pIterator->closeButtonRect.left + closeButtonImageHeight;
                pIterator->closeButtonRect.bottom = pIterator->closeButtonRect.top + closeButtonImageWidth;
            }
        } break;
        default: break; // Will never hit this.
    }


    pIterator->width  = nextTabWidth;
    pIterator->height = nextTabHeight;

    return DTK_TRUE;
}

dtk_bool32 dtk_tabbar__first_tab(const dtk_tabbar* pTabBar, dtk_tabbar__iterator* pIterator)
{
    dtk_assert(pTabBar != NULL);
    dtk_assert(pIterator != NULL);

    if (pTabBar->tabCount == 0) {
        return DTK_FALSE;   // No tabs.
    }

    dtk_zero_object(pIterator);
    pIterator->_nextIndex = -1;

    // The initial position depends on the tab flow.
    if (pTabBar->flow == dtk_tabbar_flow_right_to_left) {
        pIterator->posX = dtk_control_get_width(DTK_CONTROL(pTabBar));
    } else if (pTabBar->flow == dtk_tabbar_flow_bottom_to_top) {
        pIterator->posY = dtk_control_get_height(DTK_CONTROL(pTabBar));
    }

    if ((pTabBar->textDirection == dtk_tabbar_text_direction_horizontal && (pTabBar->flow == dtk_tabbar_flow_top_to_bottom || pTabBar->flow == dtk_tabbar_flow_bottom_to_top)) ||
        (pTabBar->textDirection == dtk_tabbar_text_direction_vertical   && (pTabBar->flow == dtk_tabbar_flow_left_to_right || pTabBar->flow == dtk_tabbar_flow_right_to_left))) {
        pIterator->_longestTextWidth = dtk_tabbar__find_longest_tab_text(pTabBar);
    }

    return dtk_tabbar__next_tab(pTabBar, pIterator);
}

dtk_bool32 dtk_tabbar__first_tab_at(const dtk_tabbar* pTabBar, dtk_int32 tabIndex, dtk_tabbar__iterator* pIterator)
{
    dtk_assert(pTabBar != NULL);
    dtk_assert(tabIndex < (dtk_int32)pTabBar->tabCount);
    dtk_assert(pIterator != NULL);

    dtk_int32 i = 0;
    if (dtk_tabbar__first_tab(pTabBar, pIterator)) {
        for (;;) {
            if (i == tabIndex) {
                return DTK_TRUE;
            }

            if (!dtk_tabbar__next_tab(pTabBar, pIterator)) {
                return DTK_FALSE;
            }

            i += 1;
        }
    }

    // Getting here means we never found the tab, or some kind of error occurred.
    return DTK_FALSE;
}


dtk_rect dtk_tabbar__get_tab_rect(const dtk_tabbar* pTabBar, dtk_int32 tabIndex)
{
    dtk_assert(pTabBar != NULL);
    dtk_assert(tabIndex < (dtk_int32)pTabBar->tabCount);

    dtk_tabbar__iterator iterator;
    if (dtk_tabbar__first_tab_at(pTabBar, tabIndex, &iterator)) {
        return dtk_rect_init(iterator.posX, iterator.posY, iterator.posX + iterator.width, iterator.posY + iterator.height);
    } else {
        return dtk_rect_init(0, 0, 0, 0);   // Couldn't find the tab somehow.
    }
}

dtk_rect dtk_tabbar__get_hovered_tab_rect(dtk_tabbar* pTabBar)
{
    dtk_assert(pTabBar != NULL);

    if (pTabBar->hoveredTabIndex == -1) {
        return dtk_rect_init(0, 0, 0, 0);
    } else {
        return dtk_tabbar__get_tab_rect(pTabBar, pTabBar->hoveredTabIndex);
    }
}


void dtk_tabbar__set_hovered_tab(dtk_tabbar* pTabBar, dtk_tabbar_hit_test_result* pHitTest)
{
    dtk_assert(pTabBar  != NULL);
    dtk_assert(pHitTest != NULL);
    dtk_assert(pHitTest->tabIndex < (dtk_int32)pTabBar->tabCount);

    // To determine what part of the tab bar needs redrawing we start out with an inside out rectangle then merge the relevant parts accordingly.
    dtk_rect redrawRect = dtk_rect_inside_out();
    
    // If the hovered tab differs we need to redraw both the previously hovered tab and the new one.
    if (pTabBar->hoveredTabIndex != pHitTest->tabIndex) {
        // The hovered tab has changed. We need only redraw the two tabs whose hovered state has changed.
        redrawRect = dtk_rect_union(redrawRect, dtk_tabbar__get_hovered_tab_rect(pTabBar));
        if (pHitTest->tabIndex != -1) {
            redrawRect = dtk_rect_union(redrawRect, pHitTest->tabRect);
        }

        pTabBar->hoveredTabIndex = pHitTest->tabIndex;

        // If the tooltip is visible, update it.
        if (pTabBar->isTooltipVisible) {
            dtk_do_tooltip(pTabBar->control.pTK);
        }
    } else {
        // The hovered tab has not changed. We may need to redraw the pin and/or close buttons, though.
        if (pTabBar->isMouseOverCloseButton != pHitTest->isOverCloseButton || pTabBar->isMouseOverPinButton != pHitTest->isOverPinButton) {
            redrawRect = dtk_rect_union(redrawRect, dtk_tabbar__get_hovered_tab_rect(pTabBar));
        }
    }

    pTabBar->isMouseOverCloseButton = pHitTest->isOverCloseButton;
    pTabBar->isMouseOverPinButton   = pHitTest->isOverPinButton;

    if (dtk_rect_has_volume(redrawRect)) {
        dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), redrawRect);
    }
}

void dtk_tabbar__unset_hovered_tab(dtk_tabbar* pTabBar)
{
    dtk_assert(pTabBar != NULL);

    dtk_rect redrawRect = dtk_rect_inside_out();
    if (pTabBar->hoveredTabIndex != -1) {
        redrawRect = dtk_rect_union(redrawRect, dtk_tabbar__get_hovered_tab_rect(pTabBar));
    }

    pTabBar->hoveredTabIndex = -1;
    pTabBar->isMouseOverCloseButton = DTK_FALSE;
    pTabBar->isMouseOverPinButton   = DTK_FALSE;
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), redrawRect);
}

void dtk_tabbar__set_active_tab(dtk_tabbar* pTabBar, dtk_int32 tabIndex, dtk_bool32 handleEventImmediately)
{
    dtk_assert(pTabBar != NULL);
    dtk_assert(tabIndex < (dtk_int32)pTabBar->tabCount);

    dtk_int32 oldTabIndex = pTabBar->activeTabIndex;
    dtk_int32 newTabIndex = tabIndex;
    if (oldTabIndex != newTabIndex) {
        dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_CHANGE_TAB, DTK_CONTROL(pTabBar));
        e.tabbar.newTabIndex = newTabIndex;
        e.tabbar.oldTabIndex = oldTabIndex;

        if (handleEventImmediately) {
            dtk_control_handle_event(DTK_CONTROL(pTabBar), &e);
        } else {
            dtk_control_post_event(DTK_CONTROL(pTabBar), &e);
        }
        

        // The actual hiding/showing of the tab pages and redrawing is done in the default event handler. The reason for this is that
        // it allows a custom event handler to cancel the tab change by simply not posting the event to the default event handler.
    }
}

void dtk_tabbar__unset_active_tab(dtk_tabbar* pTabBar, dtk_bool32 handleEventImmediately)
{
    dtk_tabbar__set_active_tab(pTabBar, -1, handleEventImmediately);
}


dtk_bool32 dtk_tabbar__is_flow_horizontal(dtk_tabbar* pTabBar)
{
    dtk_assert(pTabBar != NULL);
    return pTabBar->flow == dtk_tabbar_flow_left_to_right || pTabBar->flow == dtk_tabbar_flow_right_to_left;
}

dtk_bool32 dtk_tabbar__is_flow_vertical(dtk_tabbar* pTabBar)
{
    dtk_assert(pTabBar != NULL);
    return pTabBar->flow == dtk_tabbar_flow_top_to_bottom || pTabBar->flow == dtk_tabbar_flow_bottom_to_top;
}


dtk_result dtk_tabbar__post_event__close_tab(dtk_tabbar* pTabBar, dtk_uint32 tabIndex)
{
    dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_CLOSE_TAB, DTK_CONTROL(pTabBar));
    e.tabbar.tabIndex = tabIndex;
    return dtk_control_post_event(DTK_CONTROL(pTabBar), &e);
}


dtk_result dtk_tabbar_init(dtk_context* pTK, dtk_event_proc onEvent, dtk_control* pParent, dtk_tabbar_flow flow, dtk_tabbar_text_direction textDirection, dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;
    dtk_zero_object(pTabBar);

    dtk_result result = dtk_control_init(pTK, DTK_CONTROL_TYPE_TABBAR, (onEvent != NULL) ? onEvent : dtk_tabbar_default_event_handler, pParent, &pTabBar->control);
    if (result != DTK_SUCCESS) {
        return result;
    }

    pTabBar->flow                     = flow;
    pTabBar->textDirection            = textDirection;
    pTabBar->hoveredTabIndex          = -1;
    pTabBar->activeTabIndex           = -1;
    pTabBar->closeButtonHeldTabIndex  = -1;
    pTabBar->pinButtonHeldTabIndex    = -1;

    // Default style.
    pTabBar->bgColor                  = dtk_rgb(192, 192, 192);
    pTabBar->bgColorTab               = dtk_rgb(192, 192, 192);
    pTabBar->bgColorActiveTab         = dtk_rgb(128, 128, 128);
    pTabBar->bgColorHoveredTab        = dtk_rgb(160, 160, 160);
    pTabBar->textColor                = dtk_rgb(0, 0, 0);
    pTabBar->textColorActive          = dtk_rgb(0, 0, 0);
    pTabBar->textColorHovered         = dtk_rgb(0, 0, 0);
    pTabBar->paddingLeft              = 4;
    pTabBar->paddingTop               = 4;
    pTabBar->paddingRight             = 4;
    pTabBar->paddingBottom            = 4;
    pTabBar->closeButtonPaddingLeft   = 4;
    pTabBar->closeButtonPaddingTop    = 0;
    pTabBar->closeButtonPaddingRight  = 0;
    pTabBar->closeButtonPaddingBottom = 0;
    pTabBar->closeButtonColor         = dtk_rgb(224, 224, 224);
    pTabBar->closeButtonColorHovered  = dtk_rgb(255, 192, 192);
    pTabBar->closeButtonColorPressed  = dtk_rgb(192, 128, 128);
    pTabBar->pinButtonPaddingLeft     = 4;
    pTabBar->pinButtonPaddingTop      = 0;
    pTabBar->pinButtonPaddingRight    = 0;
    pTabBar->pinButtonPaddingBottom   = 0;
    pTabBar->pinButtonColor           = dtk_rgb(224, 224, 224);
    pTabBar->pinButtonColorHovered    = dtk_rgb(255, 192, 192);
    pTabBar->pinButtonColorPressed    = dtk_rgb(192, 128, 128);

    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_uninit(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    return dtk_control_uninit(&pTabBar->control);
}


dtk_bool32 dtk_tabbar_default_event_handler(dtk_event* pEvent)
{
    if (pEvent == NULL) return DTK_FALSE;

    dtk_tabbar* pTabBar = DTK_TABBAR(pEvent->pControl);
    dtk_assert(pTabBar != NULL);

    switch (pEvent->type)
    {
        case DTK_EVENT_PAINT:
        {
            float uiScale = dtk_control_get_scaling_factor(DTK_CONTROL(pTabBar));

            dtk_uint32 paddingLeftScaled   = (dtk_uint32)(pTabBar->paddingLeft   * uiScale);
            dtk_uint32 paddingTopScaled    = (dtk_uint32)(pTabBar->paddingTop    * uiScale);
            dtk_uint32 paddingRightScaled  = (dtk_uint32)(pTabBar->paddingRight  * uiScale);
            dtk_uint32 paddingBottomScaled = (dtk_uint32)(pTabBar->paddingBottom * uiScale);

            dtk_font* pFont = dtk_tabbar_get_font(pTabBar);
            dtk_image* pCloseButtonImage = dtk_tabbar_get_close_button_image(pTabBar);

            dtk_font_metrics fontMetrics;
            dtk_font_get_metrics(pFont, uiScale, &fontMetrics);

            dtk_rect tabGroupRect = dtk_rect_inside_out();

            dtk_int32 tabIndex = 0;
            dtk_tabbar__iterator iterator;
            if (dtk_tabbar__first_tab(pTabBar, &iterator)) {
                do
                {
                    dtk_color bgColor = pTabBar->bgColorTab;
                    dtk_color fgColor = pTabBar->textColor;
                    dtk_color closeButtonColor = pTabBar->closeButtonColor;
                    if (tabIndex == pTabBar->hoveredTabIndex) {
                        bgColor = pTabBar->bgColorHoveredTab;
                        fgColor = pTabBar->textColorHovered;
                        closeButtonColor = pTabBar->closeButtonColorTabHovered;
                    }
                    if (tabIndex == pTabBar->activeTabIndex) {
                        bgColor = pTabBar->bgColorActiveTab;
                        fgColor = pTabBar->textColorActive;
                        closeButtonColor = pTabBar->closeButtonColorTabActive;
                    }

                    if (tabIndex == pTabBar->hoveredTabIndex) {
                        if (pTabBar->isMouseOverCloseButton) {
                            closeButtonColor = pTabBar->closeButtonColorHovered;

                            if (tabIndex == pTabBar->closeButtonHeldTabIndex) {
                                closeButtonColor = pTabBar->closeButtonColorPressed;
                            }
                        }
                    }


                    dtk_rect tabRect = dtk_rect_init(iterator.posX, iterator.posY, iterator.posX + iterator.width, iterator.posY + iterator.height);
                    tabGroupRect = dtk_rect_union(tabGroupRect, tabRect);


                    // Padding area.
                    dtk_rect paddingRectLeft   = dtk_rect_init(tabRect.left,                       tabRect.top,                          tabRect.left  + paddingLeftScaled,  tabRect.bottom);
                    dtk_rect paddingRectRight  = dtk_rect_init(tabRect.right - paddingRightScaled, tabRect.top,                          tabRect.right,                      tabRect.bottom);
                    dtk_rect paddingRectTop    = dtk_rect_init(tabRect.left  + paddingLeftScaled,  tabRect.top,                          tabRect.right - paddingRightScaled, tabRect.top + paddingTopScaled);
                    dtk_rect paddingRectBottom = dtk_rect_init(tabRect.left  + paddingLeftScaled,  tabRect.bottom - paddingBottomScaled, tabRect.right - paddingRightScaled, tabRect.bottom);
                    dtk_surface_draw_rect(pEvent->paint.pSurface, paddingRectLeft,   bgColor);
                    dtk_surface_draw_rect(pEvent->paint.pSurface, paddingRectRight,  bgColor);
                    dtk_surface_draw_rect(pEvent->paint.pSurface, paddingRectTop,    bgColor);
                    dtk_surface_draw_rect(pEvent->paint.pSurface, paddingRectBottom, bgColor);

                    if (pTabBar->textDirection == dtk_tabbar_text_direction_horizontal) {
                        // Horizontal text.
                        dtk_surface_draw_text(pEvent->paint.pSurface, pFont, uiScale, iterator.pTab->pText, strlen(iterator.pTab->pText), iterator.posX + iterator.textRect.left, iterator.posY + iterator.textRect.top, fgColor, bgColor);

                        // Spacing above and below the text.
                        {
                            dtk_rect spacingRect;

                            // Above the text.
                            spacingRect.left   = iterator.posX + iterator.textRect.left;
                            spacingRect.top    = iterator.posY + paddingTopScaled;
                            spacingRect.right  = iterator.posX + iterator.textRect.right;
                            spacingRect.bottom = iterator.posY + iterator.textRect.top;
                            dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);

                            // Below the text.
                            spacingRect.left   = iterator.posX + iterator.textRect.left;
                            spacingRect.top    = iterator.posY + iterator.textRect.bottom;
                            spacingRect.right  = iterator.posX + iterator.textRect.right;
                            spacingRect.bottom = iterator.posY + iterator.height - paddingBottomScaled;
                            dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);
                        }

                        dtk_int32 cursorPosX = iterator.posX + iterator.textRect.right;

                        if (dtk_tabbar_is_showing_close_button(pTabBar)) {
                            // Spacing between text and the close button.
                            dtk_rect spacingRect;
                            spacingRect.left   = cursorPosX;
                            spacingRect.top    = iterator.posY + paddingTopScaled;
                            spacingRect.right  = iterator.posX + iterator.closeButtonRect.left;
                            spacingRect.bottom = iterator.posY + iterator.height - paddingBottomScaled;
                            dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);
                            
                            // The close button image.
                            dtk_draw_image_args drawImageArgs;
                            dtk_zero_object(&drawImageArgs);
                            drawImageArgs.dstX = iterator.posX + iterator.closeButtonRect.left;
                            drawImageArgs.dstY = iterator.posY + iterator.closeButtonRect.top;
                            drawImageArgs.dstWidth = dtk_rect_width(iterator.closeButtonRect);
                            drawImageArgs.dstHeight = dtk_rect_height(iterator.closeButtonRect);
                            drawImageArgs.srcX = 0;
                            drawImageArgs.srcY = 0;
                            drawImageArgs.srcWidth = dtk_image_get_width(pCloseButtonImage);
                            drawImageArgs.srcHeight = dtk_image_get_height(pCloseButtonImage);
                            drawImageArgs.foregroundColor = closeButtonColor;
                            drawImageArgs.backgroundColor = bgColor;
                            dtk_surface_draw_image(pEvent->paint.pSurface, pCloseButtonImage, &drawImageArgs);

                            // Spacing above and below the image.
                            {
                                // Above the text.
                                spacingRect.left   = iterator.posX + iterator.closeButtonRect.left;
                                spacingRect.top    = iterator.posY + paddingTopScaled;
                                spacingRect.right  = iterator.posX + iterator.closeButtonRect.right;
                                spacingRect.bottom = iterator.posY + iterator.closeButtonRect.top;
                                dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);

                                // Below the text.
                                spacingRect.left   = iterator.posX + iterator.closeButtonRect.left;
                                spacingRect.top    = iterator.posY + iterator.closeButtonRect.bottom;
                                spacingRect.right  = iterator.posX + iterator.closeButtonRect.right;
                                spacingRect.bottom = iterator.posY + iterator.height - paddingBottomScaled;
                                dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);
                            }

                            // Update the cursor position for the next parts.
                            cursorPosX = iterator.posX + iterator.closeButtonRect.right;
                        }

                        dtk_rect excessRect = dtk_rect_init(
                            cursorPosX,
                            iterator.posY + paddingTopScaled,
                            iterator.posX + iterator.width  - paddingRightScaled,
                            iterator.posY + iterator.height - paddingBottomScaled);
                        dtk_surface_draw_rect(pEvent->paint.pSurface, excessRect, bgColor);
                    } else {
                        // Vertical text.
                        dtk_surface_push(pEvent->paint.pSurface);
                        {
                            dtk_surface_translate(pEvent->paint.pSurface, iterator.posX + fontMetrics.lineHeight + paddingLeftScaled, iterator.posY + paddingTopScaled);
                            dtk_surface_rotate(pEvent->paint.pSurface, 90);
                            dtk_surface_draw_text(pEvent->paint.pSurface, pFont, uiScale, iterator.pTab->pText, strlen(iterator.pTab->pText), 0, 0, fgColor, bgColor);
                        }
                        dtk_surface_pop(pEvent->paint.pSurface);

                        // Spacing to the left and right the text.
                        {
                            dtk_rect spacingRect;

                            // Left of the text.
                            spacingRect.left   = iterator.posX + paddingLeftScaled;
                            spacingRect.top    = iterator.posY + iterator.textRect.top;
                            spacingRect.right  = iterator.posX + iterator.textRect.left;
                            spacingRect.bottom = iterator.posY + iterator.textRect.bottom;
                            dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);
                            
                            // Right of the text.
                            spacingRect.left   = iterator.posX + iterator.textRect.right;
                            spacingRect.top    = iterator.posY + iterator.textRect.top;
                            spacingRect.right  = iterator.posX + iterator.width - paddingRightScaled;
                            spacingRect.bottom = iterator.posY + iterator.height - paddingBottomScaled;
                            dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);
                        }

                        dtk_int32 cursorPosY = iterator.posY + iterator.textRect.bottom;

                        if (dtk_tabbar_is_showing_close_button(pTabBar)) {
                            // Spacing between text and the close button.
                            dtk_rect spacingRect;
                            spacingRect.left   = iterator.posX + paddingLeftScaled;
                            spacingRect.top    = cursorPosY;
                            spacingRect.right  = iterator.posX + iterator.width - paddingRightScaled;
                            spacingRect.bottom = iterator.posY + iterator.closeButtonRect.top;
                            dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);
                            
                            // The close button image.
                            dtk_draw_image_args drawImageArgs;
                            dtk_zero_object(&drawImageArgs);
                            drawImageArgs.dstX = 0;
                            drawImageArgs.dstY = 0;
                            drawImageArgs.dstWidth = dtk_rect_width(iterator.closeButtonRect);
                            drawImageArgs.dstHeight = dtk_rect_height(iterator.closeButtonRect);
                            drawImageArgs.srcX = 0;
                            drawImageArgs.srcY = 0;
                            drawImageArgs.srcWidth = dtk_image_get_width(pCloseButtonImage);
                            drawImageArgs.srcHeight = dtk_image_get_height(pCloseButtonImage);
                            drawImageArgs.foregroundColor = closeButtonColor;
                            drawImageArgs.backgroundColor = bgColor;

                            dtk_surface_push(pEvent->paint.pSurface);
                            {
                                dtk_surface_translate(pEvent->paint.pSurface, iterator.posX + iterator.closeButtonRect.left + dtk_rect_height(iterator.closeButtonRect), iterator.posY + iterator.closeButtonRect.top);
                                dtk_surface_rotate(pEvent->paint.pSurface, 90);
                                dtk_surface_draw_image(pEvent->paint.pSurface, pCloseButtonImage, &drawImageArgs);
                            }
                            dtk_surface_pop(pEvent->paint.pSurface);

                            // Spacing to the left and right the image.
                            {
                                // Left of the text.
                                spacingRect.left   = iterator.posX + paddingLeftScaled;
                                spacingRect.top    = iterator.posY + iterator.closeButtonRect.top;
                                spacingRect.right  = iterator.posX + iterator.closeButtonRect.left;
                                spacingRect.bottom = iterator.posY + iterator.closeButtonRect.bottom;
                                dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);
                            
                                // Right of the text.
                                spacingRect.left   = iterator.posX + iterator.closeButtonRect.right;
                                spacingRect.top    = iterator.posY + iterator.closeButtonRect.top;
                                spacingRect.right  = iterator.posX + iterator.width - paddingRightScaled;
                                spacingRect.bottom = iterator.posY + iterator.height - paddingBottomScaled;
                                dtk_surface_draw_rect(pEvent->paint.pSurface, spacingRect, bgColor);
                            }

                            // Update the cursor position for the next parts.
                            cursorPosY = iterator.posY + iterator.closeButtonRect.bottom;
                        }

                        dtk_rect excessRect = dtk_rect_init(
                            iterator.posX + paddingLeftScaled,
                            iterator.posY + cursorPosY,
                            iterator.posX + iterator.width  - paddingRightScaled,
                            iterator.posY + iterator.height - paddingBottomScaled);
                        dtk_surface_draw_rect(pEvent->paint.pSurface, excessRect, bgColor);
                    }

                    tabIndex += 1;
                } while (dtk_tabbar__next_tab(pTabBar, &iterator));
            }

            // Now we need to draw the background of the main control.
            dtk_rect tabbarRect = dtk_control_get_local_rect(DTK_CONTROL(pTabBar));
            dtk_rect bgRectLeft   = dtk_rect_init(tabbarRect.left,                      tabbarRect.top,                       tabbarRect.left + tabGroupRect.left,  tabbarRect.bottom);
            dtk_rect bgRectRight  = dtk_rect_init(tabbarRect.left + tabGroupRect.right, tabbarRect.top,                       tabbarRect.right,                     tabbarRect.bottom);
            dtk_rect bgRectTop    = dtk_rect_init(tabbarRect.left + tabGroupRect.left,  tabbarRect.top,                       tabbarRect.left + tabGroupRect.right, tabbarRect.top + tabGroupRect.top);
            dtk_rect bgRectBottom = dtk_rect_init(tabbarRect.left + tabGroupRect.left,  tabbarRect.top + tabGroupRect.bottom, tabbarRect.left + tabGroupRect.right, tabbarRect.bottom);
            dtk_surface_draw_rect(pEvent->paint.pSurface, bgRectLeft,   pTabBar->bgColor);
            dtk_surface_draw_rect(pEvent->paint.pSurface, bgRectRight,  pTabBar->bgColor);
            dtk_surface_draw_rect(pEvent->paint.pSurface, bgRectTop,    pTabBar->bgColor);
            dtk_surface_draw_rect(pEvent->paint.pSurface, bgRectBottom, pTabBar->bgColor);
        } break;

        case DTK_EVENT_MOUSE_LEAVE:
        {
            dtk_tabbar__unset_hovered_tab(pTabBar);
            pTabBar->isTooltipVisible = DTK_FALSE;
        } break;

        case DTK_EVENT_MOUSE_MOVE:
        {
            // If the tab bar has the mouse capture, don't change the hovered state of anything.
            if (!dtk_control_has_mouse_capture(DTK_CONTROL(pTabBar))) {
                dtk_tabbar_hit_test_result hit;
                if (dtk_tabbar_hit_test(pTabBar, pEvent->mouseMove.x, pEvent->mouseMove.y, &hit)) {
                    // It's over a tab.
                    dtk_tabbar__set_hovered_tab(pTabBar, &hit);
                } else {
                    // It's not over a tab.
                    dtk_tabbar__unset_hovered_tab(pTabBar);
                }
            }
        } break;

        case DTK_EVENT_MOUSE_BUTTON_DOWN:
        {
            dtk_control_capture_mouse(DTK_CONTROL(pTabBar));

            dtk_tabbar_hit_test_result hit;
            if (dtk_tabbar_hit_test(pTabBar, pEvent->mouseButton.x, pEvent->mouseButton.y, &hit)) {
                // If the user clicked on the button, do _not_ activate the tab. Instead just mark the button as held.
                if (pEvent->mouseButton.button == DTK_MOUSE_BUTTON_LEFT && (hit.isOverCloseButton || hit.isOverPinButton)) {
                    // The user clicked a button. We just mark the appropriate one as held.
                    if (hit.isOverCloseButton) {
                        pTabBar->closeButtonHeldTabIndex = hit.tabIndex;
                        dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), hit.tabRect);
                    } else if (hit.isOverPinButton) {
                        pTabBar->pinButtonHeldTabIndex = hit.tabIndex;
                        dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), hit.tabRect);
                    }
                } else {
                    // The user clicked on the main part of the control. Post a mouse button down event for the relevant tab. The tab will be activated
                    // in the DTK_EVENT_TABBAR_MOUSE_BUTTON_DOWN_TAB event handler.
                    dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_MOUSE_BUTTON_DOWN_TAB, DTK_CONTROL(pTabBar));
                    e.tabbar.tabIndex = hit.tabIndex;
                    e.tabbar.mouseButton.x = hit.relativePosX;
                    e.tabbar.mouseButton.y = hit.relativePosY;
                    e.tabbar.mouseButton.button = pEvent->mouseButton.button;
                    e.tabbar.mouseButton.state = pEvent->mouseButton.state;
                    dtk_control_handle_event(DTK_CONTROL(pTabBar), &e);
                }
            }
        } break;

        case DTK_EVENT_MOUSE_BUTTON_UP:
        {
            dtk_control_release_mouse(DTK_CONTROL(pTabBar));

            dtk_tabbar_hit_test_result hit;
            if (dtk_tabbar_hit_test(pTabBar, pEvent->mouseButton.x, pEvent->mouseButton.y, &hit)) {
                if (hit.isOverCloseButton) {
                    if (pTabBar->closeButtonHeldTabIndex == hit.tabIndex) {
                        dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_CLOSE_TAB, DTK_CONTROL(pTabBar));
                        e.tabbar.tabIndex = hit.tabIndex;
                        dtk_control_post_event(DTK_CONTROL(pTabBar), &e);
                    }
                } else if (hit.isOverPinButton) {
                    if (pTabBar->pinButtonHeldTabIndex == hit.tabIndex) {
                        
                    }
                } else {
                    dtk_tabbar__set_hovered_tab(pTabBar, &hit);

                    // Let the application know that the mouse button was released.
                    dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_MOUSE_BUTTON_UP_TAB, DTK_CONTROL(pTabBar));
                    e.tabbar.tabIndex = hit.tabIndex;
                    e.tabbar.mouseButton.x = hit.relativePosX;
                    e.tabbar.mouseButton.y = hit.relativePosY;
                    e.tabbar.mouseButton.button = pEvent->mouseButton.button;
                    e.tabbar.mouseButton.state = pEvent->mouseButton.state;
                    dtk_control_handle_event(DTK_CONTROL(pTabBar), &e);
                }
            } else {
                dtk_tabbar__unset_hovered_tab(pTabBar);
            }

            pTabBar->closeButtonHeldTabIndex = -1;
            pTabBar->pinButtonHeldTabIndex   = -1;

            dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
        } break;

        case DTK_EVENT_MOUSE_BUTTON_DBLCLICK:
        {
            dtk_tabbar_hit_test_result hit;
            if (dtk_tabbar_hit_test(pTabBar, pEvent->mouseButton.x, pEvent->mouseButton.y, &hit)) {
                dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_MOUSE_BUTTON_DBLCLICK_TAB, DTK_CONTROL(pTabBar));
                e.tabbar.tabIndex = hit.tabIndex;
                e.tabbar.mouseButton.x = hit.relativePosX;
                e.tabbar.mouseButton.y = hit.relativePosY;
                e.tabbar.mouseButton.button = pEvent->mouseButton.button;
                e.tabbar.mouseButton.state = pEvent->mouseButton.state;
                dtk_control_handle_event(DTK_CONTROL(pTabBar), &e);
            }
        } break;

        case DTK_EVENT_TOOLTIP:
        {
            dtk_tabbar_hit_test_result hit;
            if (dtk_tabbar_hit_test(pTabBar, pEvent->tooltip.x, pEvent->tooltip.y, &hit)) {
                dtk_string tooltipText = pTabBar->pTabs[hit.tabIndex].pTooltipText;
                if (!dtk_string_is_null_or_empty(tooltipText)) {
                    dtk_tooltip_show(&pEvent->tooltip.tooltip, tooltipText, hit.tabRect.left + pTabBar->control.absolutePosX, hit.tabRect.bottom + 2 + pTabBar->control.absolutePosY);
                }
            }

            pTabBar->isTooltipVisible = dtk_tooltip_is_visible(&pEvent->tooltip.tooltip);
        } break;


        case DTK_EVENT_TABBAR_MOUSE_BUTTON_DOWN_TAB:
        {
            // Actiate the tab, but not if we're going to try closing it.
            dtk_bool32 wantToClose = (pEvent->tabbar.mouseButton.button == DTK_MOUSE_BUTTON_MIDDLE && dtk_tabbar_is_close_on_middle_click_enabled(pTabBar));
            if (wantToClose) {
                if (pEvent->tabbar.mouseButton.button == DTK_MOUSE_BUTTON_MIDDLE && dtk_tabbar_is_close_on_middle_click_enabled(pTabBar)) {
                    dtk_tabbar__post_event__close_tab(pTabBar, pEvent->tabbar.tabIndex);
                }
            } else {
                dtk_tabbar__set_active_tab(pTabBar, pEvent->tabbar.tabIndex, DTK_TRUE);
            }
        } break;

        case DTK_EVENT_TABBAR_MOUSE_BUTTON_UP_TAB:
        {
        } break;

        case DTK_EVENT_TABBAR_MOUSE_BUTTON_DBLCLICK_TAB:
        {
        } break;

        case DTK_EVENT_TABBAR_CHANGE_TAB:
        {
            dtk_rect redrawRect = dtk_rect_inside_out();

            if (pEvent->tabbar.oldTabIndex != -1) {
                dtk_control_hide(pTabBar->pTabs[pEvent->tabbar.oldTabIndex].pPage);
                redrawRect = dtk_rect_union(redrawRect, dtk_tabbar__get_tab_rect(pTabBar, pEvent->tabbar.oldTabIndex));
            }
            if (pEvent->tabbar.newTabIndex != -1) {
                dtk_control_show(pTabBar->pTabs[pEvent->tabbar.newTabIndex].pPage);
                redrawRect = dtk_rect_union(redrawRect, dtk_tabbar__get_tab_rect(pTabBar, pEvent->tabbar.newTabIndex));
            }

            pTabBar->activeTabIndex = pEvent->tabbar.newTabIndex;
            dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), redrawRect);
        } break;

        case DTK_EVENT_TABBAR_CLOSE_TAB:
        {
            // Don't do anything by default. This should be controlled by the application because they may want to do things like show a confirmation dialog or whatnot.
            //dtk_tabbar_remove_tab_by_index(pTabBar, pEvent->tabbar.tabIndex);   // <-- TODO: Remove this. Only here for testing.
        } break;

        case DTK_EVENT_TABBAR_PIN_TAB:
        {
            pTabBar->pTabs[pEvent->tabbar.tabIndex].isPinned = DTK_TRUE;

            // TODO: Pin the tab.
        } break;

        case DTK_EVENT_TABBAR_UNPIN_TAB:
        {
            pTabBar->pTabs[pEvent->tabbar.tabIndex].isPinned = DTK_FALSE;

            // TODO: Unpin the tab.
        } break;

        case DTK_EVENT_TABBAR_REMOVE_TAB:
        {
            // If the tab being removed is the active tab we need to reactivate a new tab to ensure everything is in a good state. We just try
            // activating the tab to the right first. If there is nothing to the right, we do the left. Otherwise we just set it to nothing
            if ((dtk_int32)pEvent->tabbar.tabIndex == pTabBar->activeTabIndex) {
                if (pEvent->tabbar.tabIndex < (dtk_int32)pTabBar->tabCount-1) {
                    dtk_tabbar__set_active_tab(pTabBar, pEvent->tabbar.tabIndex+1, DTK_TRUE);
                } else if (pEvent->tabbar.tabIndex > 0) {
                    dtk_tabbar__set_active_tab(pTabBar, pEvent->tabbar.tabIndex-1, DTK_TRUE);
                } else {
                    dtk_tabbar__unset_active_tab(pTabBar, DTK_TRUE);
                }
            }

            // As above we need to clear the hovered tab. Only this time we don't bother with the right/left rule - we just clear it.
            if ((dtk_int32)pEvent->tabbar.tabIndex == pTabBar->hoveredTabIndex) {
                dtk_tabbar__unset_hovered_tab(pTabBar);
            }


            // Since the tabs are about to be moved around, some indices need to be adjusted to compensate.
            if (pTabBar->activeTabIndex > pEvent->tabbar.tabIndex) {
                pTabBar->activeTabIndex -= 1;
            }
            if (pTabBar->hoveredTabIndex > pEvent->tabbar.tabIndex) {
                pTabBar->hoveredTabIndex -= 1;
            }
            if (pTabBar->closeButtonHeldTabIndex > pEvent->tabbar.tabIndex) {
                pTabBar->closeButtonHeldTabIndex -= 1;
            }
            if (pTabBar->pinButtonHeldTabIndex > pEvent->tabbar.tabIndex) {
                pTabBar->pinButtonHeldTabIndex -= 1;
            }

            // Remove the tab.
            dtk_tabbar_tab_uninit(&pTabBar->pTabs[pEvent->tabbar.tabIndex]);
            for (dtk_uint32 i = pEvent->tabbar.tabIndex; i < pTabBar->tabCount-1; ++i) {
                pTabBar->pTabs[i] = pTabBar->pTabs[i+1];
            }
            pTabBar->tabCount -= 1;


            // Resize.
            dtk_tabbar_try_auto_resize(pTabBar);

            // Redraw.
            dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
        } break;

        default: break;
    }

    return dtk_control_default_event_handler(pEvent);
}


dtk_result dtk_tabbar_set_bg_color(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->bgColor = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_font(dtk_tabbar* pTabBar, dtk_font* pFont)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->pFont = pFont;

    dtk_tabbar_try_auto_resize(pTabBar);
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_font* dtk_tabbar_get_font(const dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return NULL;
    return (pTabBar->pFont != NULL) ? pTabBar->pFont : dtk_get_ui_font(pTabBar->control.pTK);
}

dtk_result dtk_tabbar_set_close_button_image(dtk_tabbar* pTabBar, dtk_image* pImage)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->pCloseButtonImage = pImage;

    dtk_tabbar_try_auto_resize(pTabBar);
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_image* dtk_tabbar_get_close_button_image(const dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return NULL;
    return (pTabBar->pCloseButtonImage != NULL) ? pTabBar->pCloseButtonImage : dtk_get_stock_image(DTK_CONTROL(pTabBar)->pTK, DTK_STOCK_IMAGE_CROSS);
}

dtk_result dtk_tabbar_set_close_button_color(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->closeButtonColor = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_close_button_color_tab_hovered(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->closeButtonColorTabHovered = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_close_button_color_tab_active(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->closeButtonColorTabActive = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_close_button_color_hovered(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->closeButtonColorHovered = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_close_button_color_pressed(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->closeButtonColorPressed = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_text_color(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->textColor = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_text_color_active(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->textColorActive = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_text_color_hovered(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->textColorHovered = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_tab_bg_color(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->bgColorTab = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_tab_bg_color_active(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->bgColorActiveTab = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_tab_bg_color_hovered(dtk_tabbar* pTabBar, dtk_color color)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->bgColorHoveredTab = color;
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_tab_padding(dtk_tabbar* pTabBar, dtk_uint32 paddingLeft, dtk_uint32 paddingTop, dtk_uint32 paddingRight, dtk_uint32 paddingBottom)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->paddingLeft   = paddingLeft;
    pTabBar->paddingTop    = paddingTop;
    pTabBar->paddingRight  = paddingRight;
    pTabBar->paddingBottom = paddingBottom;
    
    dtk_tabbar_try_auto_resize(pTabBar);
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_close_button_padding(dtk_tabbar* pTabBar, dtk_uint32 paddingLeft, dtk_uint32 paddingTop, dtk_uint32 paddingRight, dtk_uint32 paddingBottom)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->closeButtonPaddingLeft   = paddingLeft;
    pTabBar->closeButtonPaddingTop    = paddingTop;
    pTabBar->closeButtonPaddingRight  = paddingRight;
    pTabBar->closeButtonPaddingBottom = paddingBottom;
    
    dtk_tabbar_try_auto_resize(pTabBar);
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_pin_button_padding(dtk_tabbar* pTabBar, dtk_uint32 paddingLeft, dtk_uint32 paddingTop, dtk_uint32 paddingRight, dtk_uint32 paddingBottom)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->pinButtonPaddingLeft   = paddingLeft;
    pTabBar->pinButtonPaddingTop    = paddingTop;
    pTabBar->pinButtonPaddingRight  = paddingRight;
    pTabBar->pinButtonPaddingBottom = paddingBottom;
    
    dtk_tabbar_try_auto_resize(pTabBar);
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_show_close_button(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    if (pTabBar->isShowingCloseButton) {
        return DTK_SUCCESS;
    }

    pTabBar->isShowingCloseButton = DTK_TRUE;

    if ((pTabBar->textDirection == dtk_tabbar_text_direction_horizontal && !dtk_tabbar__is_flow_horizontal(pTabBar)) ||
        (pTabBar->textDirection == dtk_tabbar_text_direction_vertical   && !dtk_tabbar__is_flow_vertical(pTabBar))) {
        dtk_tabbar_try_auto_resize(pTabBar);
    }

    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_hide_close_button(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    if (!pTabBar->isShowingCloseButton) {
        return DTK_SUCCESS;
    }

    pTabBar->isShowingCloseButton = DTK_FALSE;

    if ((pTabBar->textDirection == dtk_tabbar_text_direction_horizontal && !dtk_tabbar__is_flow_horizontal(pTabBar)) ||
        (pTabBar->textDirection == dtk_tabbar_text_direction_vertical   && !dtk_tabbar__is_flow_vertical(pTabBar))) {
        dtk_tabbar_try_auto_resize(pTabBar);
    }

    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_bool32 dtk_tabbar_is_showing_close_button(const dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return DTK_FALSE;

    return pTabBar->isShowingCloseButton && dtk_tabbar_get_close_button_image(pTabBar) != NULL;
}

dtk_result dtk_tabbar_set_close_button_size(dtk_tabbar* pTabBar, dtk_uint32 width, dtk_uint32 height)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    pTabBar->closeButtonWidth = width;
    pTabBar->closeButtonHeight = height;

    if ((pTabBar->textDirection == dtk_tabbar_text_direction_horizontal && !dtk_tabbar__is_flow_horizontal(pTabBar)) ||
        (pTabBar->textDirection == dtk_tabbar_text_direction_vertical   && !dtk_tabbar__is_flow_vertical(pTabBar))) {
        dtk_tabbar_try_auto_resize(pTabBar);
    }
    
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_get_close_button_size(const dtk_tabbar* pTabBar, dtk_uint32* pWidth, dtk_uint32* pHeight)
{
    if (pWidth) {
        *pWidth = dtk_tabbar_get_close_button_width(pTabBar);
    }
    if (pHeight) {
        *pHeight = dtk_tabbar_get_close_button_height(pTabBar);
    }

    return DTK_SUCCESS;
}

dtk_uint32 dtk_tabbar_get_close_button_width(const dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return 0;

    if (dtk_tabbar_get_close_button_image(pTabBar) == NULL) {
        return 0;
    }

    if (pTabBar->closeButtonWidth == 0) {
        return dtk_image_get_width(dtk_tabbar_get_close_button_image(pTabBar));
    }

    return pTabBar->closeButtonWidth;
}

dtk_uint32 dtk_tabbar_get_close_button_height(const dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) return 0;

    if (dtk_tabbar_get_close_button_image(pTabBar) == NULL) {
        return 0;
    }

    if (pTabBar->closeButtonHeight == 0) {
        return dtk_image_get_height(dtk_tabbar_get_close_button_image(pTabBar));
    }

    return pTabBar->closeButtonHeight;
}




dtk_result dtk_tabbar_append_tab(dtk_tabbar* pTabBar, const char* text, dtk_control* pTabPage, dtk_uint32* pTabIndexOut)
{
    if (pTabBar == NULL) return DTK_INVALID_ARGS;

    if (pTabBar->tabCount == pTabBar->tabCapacity) {
        dtk_uint32 newTabCapacity = (pTabBar->tabCapacity == 0) ? 1 : pTabBar->tabCapacity * 2;
        dtk_tabbar_tab* pNewTabs = (dtk_tabbar_tab*)dtk_realloc(pTabBar->pTabs, sizeof(*pNewTabs) * newTabCapacity);
        if (pNewTabs == NULL) {
            return DTK_OUT_OF_MEMORY;
        }

        pTabBar->pTabs = pNewTabs;
        pTabBar->tabCapacity = newTabCapacity;
    }

    dtk_assert(pTabBar->tabCapacity > pTabBar->tabCount);

    dtk_uint32 tabIndex = pTabBar->tabCount;
    if (pTabIndexOut) {
        *pTabIndexOut = tabIndex;
    }

    dtk_tabbar_tab_init(pTabBar, text, pTabPage, &pTabBar->pTabs[tabIndex]);
    pTabBar->tabCount += 1;

    dtk_tabbar_try_auto_resize(pTabBar);
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
	return DTK_SUCCESS;
}

dtk_result dtk_tabbar_prepend_tab(dtk_tabbar* pTabBar, const char* text, dtk_control* pTabPage, dtk_uint32* pTabIndexOut)
{
	if (pTabBar == NULL) return DTK_INVALID_ARGS;

    if (pTabBar->tabCount == pTabBar->tabCapacity) {
        dtk_uint32 newTabCapacity = (pTabBar->tabCapacity == 0) ? 1 : pTabBar->tabCapacity * 2;
        dtk_tabbar_tab* pNewTabs = (dtk_tabbar_tab*)dtk_realloc(pTabBar->pTabs, sizeof(*pNewTabs) * newTabCapacity);
        if (pNewTabs == NULL) {
            return DTK_OUT_OF_MEMORY;
        }

        pTabBar->pTabs = pNewTabs;
        pTabBar->tabCapacity = newTabCapacity;
    }

    dtk_assert(pTabBar->tabCapacity > pTabBar->tabCount);

    dtk_uint32 tabIndex = 0;    // Prepending always occurs at position 0 for now, but may change when pinning is implemented.
    if (pTabIndexOut) {
        *pTabIndexOut = tabIndex;
    }

    // Move everything down to make room for the new tab.
    memmove(pTabBar->pTabs + tabIndex + 1, pTabBar->pTabs + tabIndex, sizeof(*pTabBar->pTabs) * (pTabBar->tabCount-tabIndex));
    
    dtk_tabbar_tab_init(pTabBar, text, pTabPage, &pTabBar->pTabs[tabIndex]);
    pTabBar->tabCount += 1;

    // Some indices need to change.
    if (pTabBar->activeTabIndex >= (dtk_int32)tabIndex) {
        pTabBar->activeTabIndex += 1;
    }
    if (pTabBar->closeButtonHeldTabIndex >= (dtk_int32)tabIndex) {
        pTabBar->closeButtonHeldTabIndex += 1;
    }
    if (pTabBar->pinButtonHeldTabIndex >= (dtk_int32)tabIndex) {
        pTabBar->pinButtonHeldTabIndex += 1;
    }

    dtk_tabbar_try_auto_resize(pTabBar);
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
	return DTK_SUCCESS;
}

dtk_result dtk_tabbar_remove_tab_by_index(dtk_tabbar* pTabBar, dtk_uint32 tabIndex)
{
    if (pTabBar == NULL || pTabBar->tabCount <= tabIndex) return DTK_INVALID_ARGS;

    // Tab removal is event driven.
    dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_REMOVE_TAB, DTK_CONTROL(pTabBar));
    e.tabbar.tabIndex = tabIndex;
    return dtk_control_post_event(DTK_CONTROL(pTabBar), &e);
}

dtk_uint32 dtk_tabbar_get_tab_count(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return 0;
    }

    return pTabBar->tabCount;
}


dtk_control* dtk_tabbar_get_tab_page(dtk_tabbar* pTabBar, dtk_uint32 tabIndex)
{
    if (pTabBar == NULL || pTabBar->tabCount <= tabIndex) return NULL;

    return pTabBar->pTabs[tabIndex].pPage;
}

dtk_uint32 dtk_tabbar_get_active_tab_index(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL || pTabBar->tabCount == 0) {
        return (dtk_uint32)-1;
    }

    return pTabBar->activeTabIndex;
}

dtk_result dtk_tabbar_activate_tab(dtk_tabbar* pTabBar, dtk_uint32 tabIndex)
{
    if (pTabBar == NULL || pTabBar->tabCount <= tabIndex) return DTK_INVALID_ARGS;

    dtk_tabbar__set_active_tab(pTabBar, tabIndex, DTK_FALSE);
    return DTK_SUCCESS;
}


dtk_result dtk_tabbar_activate_next_tab(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL || pTabBar->tabCount == 0) {
        return DTK_INVALID_ARGS;
    }

    dtk_int32 newIndex = pTabBar->activeTabIndex;
    if (newIndex == -1) {
        newIndex = 0;
    }

    if (newIndex >= (dtk_int32)pTabBar->tabCount) {
        newIndex = 0;
    }

    return dtk_tabbar_activate_tab(pTabBar, newIndex);
}

dtk_result dtk_tabbar_activate_prev_tab(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL || pTabBar->tabCount == 0) {
        return DTK_INVALID_ARGS;
    }

    dtk_int32 newIndex = pTabBar->activeTabIndex;
    if (newIndex == -1) {
        newIndex = 0;
    }

    if (newIndex == 0) {
        newIndex = pTabBar->tabCount-1;
    }

    return dtk_tabbar_activate_tab(pTabBar, newIndex);
}


dtk_result dtk_tabbar_set_tab_text(dtk_tabbar* pTabBar, dtk_uint32 tabIndex, const char* pTabText)
{
    if (pTabBar == NULL || pTabBar->tabCount <= tabIndex) return DTK_INVALID_ARGS;

    pTabBar->pTabs[tabIndex].pText = dtk_set_string(pTabBar->pTabs[tabIndex].pText, pTabText);

    dtk_tabbar_try_auto_resize(pTabBar);
    dtk_control_scheduled_redraw(DTK_CONTROL(pTabBar), dtk_control_get_local_rect(DTK_CONTROL(pTabBar)));
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_set_tab_tooltip(dtk_tabbar* pTabBar, dtk_uint32 tabIndex, const char* pTooltipText)
{
    if (pTabBar == NULL || pTabBar->tabCount <= tabIndex) return DTK_INVALID_ARGS;

    pTabBar->pTabs[tabIndex].pTooltipText = dtk_set_string(pTabBar->pTabs[tabIndex].pTooltipText, pTooltipText);
    return DTK_SUCCESS;
}


dtk_result dtk_tabbar_pin_tab(dtk_tabbar* pTabBar, dtk_uint32 tabIndex)
{
    if (pTabBar == NULL || pTabBar->tabCount <= tabIndex) return DTK_INVALID_ARGS;

    if (pTabBar->pTabs[tabIndex].isPinned) {
        return DTK_SUCCESS; // Already pinned.
    }

    dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_PIN_TAB, DTK_CONTROL(pTabBar));
    e.tabbar.tabIndex = tabIndex;
    return dtk_control_post_event(DTK_CONTROL(pTabBar), &e);
}

dtk_result dtk_tabbar_unpin_tab(dtk_tabbar* pTabBar, dtk_uint32 tabIndex)
{
    if (pTabBar == NULL || pTabBar->tabCount <= tabIndex) return DTK_INVALID_ARGS;

    if (!pTabBar->pTabs[tabIndex].isPinned) {
        return DTK_SUCCESS; // Already unpinned.
    }

    dtk_event e = dtk_event_init(DTK_CONTROL(pTabBar)->pTK, DTK_EVENT_TABBAR_UNPIN_TAB, DTK_CONTROL(pTabBar));
    e.tabbar.tabIndex = tabIndex;
    return dtk_control_post_event(DTK_CONTROL(pTabBar), &e);
}


dtk_bool32 dtk_tabbar_hit_test(dtk_tabbar* pTabBar, dtk_int32 x, dtk_int32 y, dtk_tabbar_hit_test_result* pResult)
{
    if (pTabBar == NULL) return DTK_FALSE;

    pResult->tabIndex = -1;
    pResult->relativePosX = 0;
    pResult->relativePosY = 0;
    pResult->isOverCloseButton = DTK_FALSE;

    dtk_int32 tabIndex = 0;
    dtk_tabbar__iterator iterator;
    if (dtk_tabbar__first_tab(pTabBar, &iterator)) {
        do
        {
            dtk_rect tabRect = dtk_rect_init(iterator.posX, iterator.posY, iterator.posX + iterator.width, iterator.posY + iterator.height);
            if (dtk_rect_contains_point(tabRect, x, y)) {
                pResult->tabIndex = tabIndex;
                pResult->relativePosX = x - iterator.posX;
                pResult->relativePosY = y - iterator.posY;
                pResult->tabRect = tabRect;

                // Check if the point is over the close button.
                pResult->isOverCloseButton = DTK_FALSE;
                if (pResult->relativePosX >= iterator.closeButtonRect.left && pResult->relativePosX < iterator.closeButtonRect.right &&
                    pResult->relativePosY >= iterator.closeButtonRect.top  && pResult->relativePosY < iterator.closeButtonRect.bottom) {
                    pResult->isOverCloseButton = DTK_TRUE;
                }

                // Check if the point is over the pin button.
                pResult->isOverPinButton = DTK_FALSE;
                if (pResult->relativePosX >= iterator.pinButtonRect.left && pResult->relativePosX < iterator.pinButtonRect.right &&
                    pResult->relativePosY >= iterator.pinButtonRect.top  && pResult->relativePosY < iterator.pinButtonRect.bottom) {
                    pResult->isOverPinButton = DTK_TRUE;
                }

                return DTK_TRUE;
            }

            tabIndex += 1;
        } while (dtk_tabbar__next_tab(pTabBar, &iterator));
    }

    return DTK_FALSE;
}


dtk_result dtk_tabbar_enable_auto_resize(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return DTK_INVALID_ARGS;
    }

    pTabBar->isAutoResizeEnabled = DTK_TRUE;
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_disable_auto_resize(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return DTK_INVALID_ARGS;
    }

    pTabBar->isAutoResizeEnabled = DTK_FALSE;
    return DTK_SUCCESS;
}

dtk_bool32 dtk_tabbar_is_auto_resize_enabled(const dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return DTK_FALSE;
    }

    return pTabBar->isAutoResizeEnabled;
}


dtk_result dtk_tabbar_auto_resize__horz(dtk_tabbar* pTabBar)
{
    dtk_assert(pTabBar != NULL);
    dtk_assert(pTabBar->flow == dtk_tabbar_flow_left_to_right || pTabBar->flow == dtk_tabbar_flow_right_to_left);

    dtk_int32 newWidth;
    dtk_int32 newHeight;
    dtk_result result = dtk_control_get_size(DTK_CONTROL(pTabBar), &newWidth, &newHeight);
    if (result != DTK_SUCCESS) {
        return result;
    }

    // The width needs to be the size as the parent.
    dtk_control* pParent = dtk_control_get_parent(DTK_CONTROL(pTabBar));
    if (pParent != NULL) {
        newWidth = dtk_control_get_width(pParent);
    }

    // The height needs to be set to the maximum height of the tabs.
    newHeight = 0;
    dtk_tabbar__iterator i;
    if (dtk_tabbar__first_tab(pTabBar, &i)) {
        do
        {
            dtk_int32 thisHeight = i.height;
            if (newHeight < thisHeight) {
                newHeight = thisHeight;
            }
        } while (dtk_tabbar__next_tab(pTabBar, &i));
    }

    // Now resize.
    return dtk_control_set_size(DTK_CONTROL(pTabBar), newWidth, newHeight);
}

dtk_result dtk_tabbar_auto_resize__vert(dtk_tabbar* pTabBar)
{
    dtk_assert(pTabBar != NULL);
    dtk_assert(pTabBar->flow == dtk_tabbar_flow_top_to_bottom || pTabBar->flow == dtk_tabbar_flow_bottom_to_top);

    dtk_int32 newWidth;
    dtk_int32 newHeight;
    dtk_result result = dtk_control_get_size(DTK_CONTROL(pTabBar), &newWidth, &newHeight);
    if (result != DTK_SUCCESS) {
        return result;
    }

    // The height needs to be the size as the parent.
    dtk_control* pParent = dtk_control_get_parent(DTK_CONTROL(pTabBar));
    if (pParent != NULL) {
        newHeight = dtk_control_get_height(pParent);
    }

    // The width needs to be set to the maximum width of the tabs.
    newWidth = 0;
    dtk_tabbar__iterator i;
    if (dtk_tabbar__first_tab(pTabBar, &i)) {
        do
        {
            dtk_int32 thisWidth = i.width;
            if (newWidth < thisWidth) {
                newWidth = thisWidth;
            }
        } while (dtk_tabbar__next_tab(pTabBar, &i));
    }

    // Now resize.
    return dtk_control_set_size(DTK_CONTROL(pTabBar), newWidth, newHeight);
}

dtk_result dtk_tabbar_auto_resize(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return DTK_INVALID_ARGS;
    }

    if (pTabBar->flow == dtk_tabbar_flow_left_to_right || pTabBar->flow == dtk_tabbar_flow_right_to_left) {
        return dtk_tabbar_auto_resize__horz(pTabBar);
    } else {
        return dtk_tabbar_auto_resize__vert(pTabBar);
    }
}

dtk_result dtk_tabbar_try_auto_resize(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return DTK_INVALID_ARGS;
    }

    if (!pTabBar->isAutoResizeEnabled) {
        return DTK_INVALID_OPERATION;
    }

    return dtk_tabbar_auto_resize(pTabBar);
}


dtk_result dtk_tabbar_enable_close_on_middle_click(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return DTK_INVALID_ARGS;
    }

    pTabBar->isCloseOnMiddleClientEnabled = DTK_TRUE;
    return DTK_SUCCESS;
}

dtk_result dtk_tabbar_disable_close_on_middle_click(dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return DTK_INVALID_ARGS;
    }

    pTabBar->isCloseOnMiddleClientEnabled = DTK_FALSE;
    return DTK_SUCCESS;
}

dtk_bool32 dtk_tabbar_is_close_on_middle_click_enabled(const dtk_tabbar* pTabBar)
{
    if (pTabBar == NULL) {
        return DTK_FALSE;
    }

    return pTabBar->isCloseOnMiddleClientEnabled;
}


dtk_result dtk_tabbar_transform_point_from_tab(const dtk_tabbar* pTabBar, dtk_uint32 tabIndex, dtk_int32* pX, dtk_int32* pY)
{
    if (pTabBar == NULL) {
        return DTK_FALSE;
    }

    dtk_tabbar__iterator iterator;
    if (!dtk_tabbar__first_tab_at(pTabBar, tabIndex, &iterator)) {
        return DTK_ERROR;
    }

    if (pX) *pX += iterator.posX;
    if (pY) *pY += iterator.posY;

    return DTK_SUCCESS;
}
