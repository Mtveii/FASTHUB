#pragma once

#include "core/i_search_provider.h"

namespace fasthub::search {

class SearchProvider : public core::ISearchProvider {
public:
    SearchProvider() = default;
    ~SearchProvider() override = default;

    std::vector<core::SearchResult> search(const std::string &query) override;
};

} // namespace fasthub::search
