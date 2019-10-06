#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <memory>
#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage
{
public:
    SimpleLRU(size_t max_size = 1024)
        :_max_size(max_size),_current_size(0), _lru_head(new lru_node())
    {

        _lru_head->next = std::unique_ptr<lru_node>(new lru_node());
        _lru_tail = _lru_head->next.get();
        _lru_tail->prev = _lru_head.get();

    }

    ~SimpleLRU()
    {

      _lru_index.clear();

      while (_lru_head->next != nullptr)
      {
          std::unique_ptr<lru_node> dn;
          dn.swap(_lru_head);
          _lru_head.swap(dn->next);
      }

      _lru_head.reset();

    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    // LRU cache node
    using lru_node = struct lru_node
    {

        const std::string key;
        std::string value;
        lru_node *prev; //fixed
        std::unique_ptr<lru_node> next;
            lru_node (const std::string &k, const std::string &v)
            :key(k), value(v), prev(nullptr), next(nullptr)
        {

        }
    //  private:
    //    friend class SimpleLRU;
        lru_node() : key(""), value(""), prev(nullptr), next(nullptr)
        {

        }
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size, _current_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;
    lru_node * _lru_tail; //tail node

    // Index of nodes from list above 
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>> _lru_index;
    bool Insert(const std::string &key, const std::string &value);
    bool Replace(lru_node &orig, const std::string &value);
    bool Delete(lru_node &delete_node);

    bool DecreaseSizeIfNeeded(size_t oldNodeSize, size_t newNodeSize);

    bool MoveLast(lru_node &node);
    bool DeleteFirst();
};
} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
