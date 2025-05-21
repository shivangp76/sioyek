#pragma once

#include <vector>
class QStandardItemModel;
class TocNode;

QStandardItemModel* create_table_model(std::vector<std::wstring> lefts, std::vector<std::wstring> rights);
QStandardItemModel* create_table_model(const std::vector<std::vector<std::wstring>> column_texts);
QStandardItemModel* get_model_from_toc(const std::vector<TocNode*>& roots);
