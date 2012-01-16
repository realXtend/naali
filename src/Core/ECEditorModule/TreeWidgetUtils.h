/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   TreeWidgetUtils.h
 *  @brief  Tree widget utility functions.
 */

#pragma once

/// Searches for items containing @c filter (case-insensitive) in the item's @c column and toggles their visibility.
/** If match is found the item is set visible and expanded (if filter >= 3 chars), otherwise it's hidden.
    If @c filter begins with '!', negation search is performed, i.e. every item containing the filter is hidden instead.
    @param treeWidget Target tree widget for the action.
    @param column Which column's text is used.
    @param filter Text used as a filter. If an empty string, all items in the tree widget are set visible. */
void TreeWidgetSearch(QTreeWidget *treeWidget, int column, const QString &filter);

/// Expands or collapses the whole tree view, depending on the previous action.
/** @param treeWidget Target tree widget for the action.
    @return bool True all items are expanded, false if all items are collapsed. */
bool TreeWidgetExpandOrCollapseAll(QTreeWidget *treeWidget);

/// Sets the check state of all items in the @c treeWidget.
/** Sorting and updates are disabled before setting the state and enabled afterwards.
    @param treeWidget Target tree widget for the action.
    @param column Column for which the @c state is set.
    @param state Check state. */
void TreeWidgetSetCheckStateForAllItems(QTreeWidget *treeWidget, int column, Qt::CheckState state);

