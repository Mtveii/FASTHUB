#pragma once

#include <string>
#include <vector>

namespace fasthub::core {

/// @brief Represents a single search match result.
struct SearchResult {
    std::string internalId;     // Format: provider:externalId or file path
    std::string source;         // "telegram", "workspace", etc.
    std::string title;          // Chat title or file name
    std::string snippet;        // Matching text snippet
};

/// @brief Abstract interface for global full-text search across providers and workspace.
class ISearchProvider {
public:
    virtual ~ISearchProvider() = default;

    /// @brief Performs full-text search query across indexed sources.
    /// @param query Search query string.
    /// @return Vector of search results matching the query.
    virtual std::vector<SearchResult> search(const std::string& query) = 0;
};

} // namespace fasthub::core
