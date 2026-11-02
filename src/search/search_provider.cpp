#include "search_provider.h"

#include "database/database_manager.h"

namespace fasthub::search {

std::vector<core::SearchResult> SearchProvider::search(const std::string &query) {
    return database::DatabaseManager::instance().searchFts(query);
}

} // namespace fasthub::search
