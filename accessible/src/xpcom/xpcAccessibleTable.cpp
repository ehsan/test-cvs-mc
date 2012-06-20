/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "xpcAccessibleTable.h"

#include "Accessible.h"
#include "TableAccessible.h"

nsresult
xpcAccessibleTable::GetCaption(nsIAccessible** aCaption)
{
  NS_ENSURE_ARG_POINTER(aCaption);
  *aCaption = nsnull;
  if (!mTable)
    return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aCaption = mTable->Caption());
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetColumnCount(PRInt32* aColumnCount)
{
  NS_ENSURE_ARG_POINTER(aColumnCount);
  *aColumnCount = 0;

  if (!mTable)
    return NS_ERROR_FAILURE;

  *aColumnCount = mTable->ColCount();
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetRowCount(PRInt32* aRowCount)
{
  NS_ENSURE_ARG_POINTER(aRowCount);
  *aRowCount = 0;

  if (!mTable)
    return NS_ERROR_FAILURE;

  *aRowCount = mTable->RowCount();
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetCellAt(PRInt32 aRowIdx, PRInt32 aColIdx,
                              nsIAccessible** aCell)
{
  NS_ENSURE_ARG_POINTER(aCell);
  *aCell = nsnull;

  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || static_cast<PRUint32>(aRowIdx) >= mTable->RowCount() ||
      aColIdx < 0 || static_cast<PRUint32>(aColIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  NS_IF_ADDREF(*aCell = mTable->CellAt(aRowIdx, aColIdx));
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetCellIndexAt(PRInt32 aRowIdx, PRInt32 aColIdx,
                                   PRInt32* aCellIdx)
{
  NS_ENSURE_ARG_POINTER(aCellIdx);
  *aCellIdx = -1;

  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || static_cast<PRUint32>(aRowIdx) >= mTable->RowCount() ||
      aColIdx < 0 || static_cast<PRUint32>(aColIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  *aCellIdx = mTable->CellIndexAt(aRowIdx, aColIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetColumnExtentAt(PRInt32 aRowIdx, PRInt32 aColIdx,
                                      PRInt32* aColumnExtent)
{
  NS_ENSURE_ARG_POINTER(aColumnExtent);
  *aColumnExtent = -1;

  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || static_cast<PRUint32>(aRowIdx) >= mTable->RowCount() ||
      aColIdx < 0 || static_cast<PRUint32>(aColIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  *aColumnExtent = mTable->ColExtentAt(aRowIdx, aColIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetRowExtentAt(PRInt32 aRowIdx, PRInt32 aColIdx,
                                   PRInt32* aRowExtent)
{
  NS_ENSURE_ARG_POINTER(aRowExtent);
  *aRowExtent = -1;

  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || static_cast<PRUint32>(aRowIdx) >= mTable->RowCount() ||
      aColIdx < 0 || static_cast<PRUint32>(aColIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  *aRowExtent = mTable->RowExtentAt(aRowIdx, aColIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetColumnDescription(PRInt32 aColIdx,
                                         nsAString& aDescription)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aColIdx < 0 || static_cast<PRUint32>(aColIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  nsAutoString description;
  mTable->ColDescription(aColIdx, description);
  aDescription.Assign(description);

  return NS_OK;
}

nsresult
xpcAccessibleTable::GetRowDescription(PRInt32 aRowIdx, nsAString& aDescription)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || static_cast<PRUint32>(aRowIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  nsAutoString description;
  mTable->RowDescription(aRowIdx, description);
  aDescription.Assign(description);

  return NS_OK;
}

nsresult
xpcAccessibleTable::IsColumnSelected(PRInt32 aColIdx, bool* aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = false;

  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aColIdx < 0 || static_cast<PRUint32>(aColIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  *aIsSelected = mTable->IsColSelected(aColIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::IsRowSelected(PRInt32 aRowIdx, bool* aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = false;

  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || static_cast<PRUint32>(aRowIdx) >= mTable->RowCount())
    return NS_ERROR_INVALID_ARG;

  *aIsSelected = mTable->IsRowSelected(aRowIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::IsCellSelected(PRInt32 aRowIdx, PRInt32 aColIdx,
                                   bool* aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = false;

  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || static_cast<PRUint32>(aRowIdx) >= mTable->RowCount() ||
      aColIdx < 0 || static_cast<PRUint32>(aColIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  *aIsSelected = mTable->IsCellSelected(aRowIdx, aColIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetSelectedCellCount(PRUint32* aSelectedCellCount)
{
  NS_ENSURE_ARG_POINTER(aSelectedCellCount);
  *aSelectedCellCount = 0;

  if (!mTable)
    return NS_ERROR_FAILURE;

  *aSelectedCellCount = mTable->SelectedCellCount();
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetSelectedColumnCount(PRUint32* aSelectedColumnCount)
{
  NS_ENSURE_ARG_POINTER(aSelectedColumnCount);
  *aSelectedColumnCount = 0;

  if (!mTable)
    return NS_ERROR_FAILURE;

  *aSelectedColumnCount = mTable->SelectedColCount();
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetSelectedRowCount(PRUint32* aSelectedRowCount)
{
  NS_ENSURE_ARG_POINTER(aSelectedRowCount);
  *aSelectedRowCount = 0;

  if (!mTable)
    return NS_ERROR_FAILURE;

  *aSelectedRowCount = mTable->SelectedRowCount();
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetSummary(nsAString& aSummary)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  nsAutoString summary;
  mTable->Summary(summary);
  aSummary.Assign(summary);

  return NS_OK;
}

nsresult
xpcAccessibleTable::IsProbablyForLayout(bool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = false;
  if (!mTable)
    return NS_ERROR_FAILURE;

  *aResult = mTable->IsProbablyLayoutTable();
  return NS_OK;
}

nsresult
xpcAccessibleTable::SelectColumn(PRInt32 aColIdx)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aColIdx < 0 || static_cast<PRUint32>(aColIdx) >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  mTable->SelectCol(aColIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::SelectRow(PRInt32 aRowIdx)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || static_cast<PRUint32>(aRowIdx) >= mTable->RowCount())
    return NS_ERROR_INVALID_ARG;

  mTable->SelectRow(aRowIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::UnselectColumn(PRInt32 aColIdx)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aColIdx < 0 || aColIdx >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  mTable->UnselectCol(aColIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::UnselectRow(PRInt32 aRowIdx)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || aRowIdx >= mTable->RowCount())
    return NS_ERROR_INVALID_ARG;

  mTable->UnselectRow(aRowIdx);
  return NS_OK;
}


