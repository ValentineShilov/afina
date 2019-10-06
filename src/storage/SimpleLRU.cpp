#include "SimpleLRU.h"
#include <iostream>
namespace Afina {
namespace Backend {


bool SimpleLRU::Put(const std::string &key, const std::string &value)
{

	if( (key.length() == 0) || (value.size() + key.size() ) > _max_size)
	{
    return false;
	}

	auto it = _lru_index.find(std::reference_wrapper<const std::string>(key));
	if (it == _lru_index.end())
	{
		return Insert(key, value);
	}
	return Replace(it->second, value);

return true;
}



bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{

  if( (key.length() == 0) || ( (value.size() + key.size()) <= _max_size) )
  {
		if(_lru_index.find(std::reference_wrapper<const std::string>(key))==_lru_index.end())
		{
			return Insert(key, value);
		}
		else
		{
			return false;
		}
	}
	return false;
 }


bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
  if((key.length() == 0) || ((value.size() + key.size() ) > _max_size) )
	{
    return false;
	}
	auto it = _lru_index.find(std::reference_wrapper<const std::string>(key));

	if( it != _lru_index.end())
	{
		return Replace(it->second, value);
	}
	else
	{
    return false;
	}
}


bool SimpleLRU::Delete(const std::string &key)
 {
	if(key.length() == 0)
	{
		return false;
	}
	auto it = _lru_index.find(std::reference_wrapper<const std::string>(key));
	if (it != _lru_index.end())
	{
	  return Delete(it->second.get());
	}

	//not found - nothing to delete
	return false;
}



bool SimpleLRU::Get(const std::string &key, std::string &value)
{


	auto n = _lru_index.find(std::reference_wrapper<const std::string>(key));

	if(n != _lru_index.end())
	{
		value = n->second.get().value;
		return MoveLast(n->second);
	}
	return false;
}

bool SimpleLRU::Insert(const std::string &key, const std::string &value)
{
	DecreaseSizeIfNeeded(0, key.size() + value.size());

	//some problem with make_unique on g++ 4.9 so constructor and new used
	auto putNodePtr = std::unique_ptr<lru_node>(new lru_node(key,value));
  putNodePtr->prev = _lru_tail->prev;
	putNodePtr->next.swap(_lru_tail->prev->next);
	_lru_tail->prev->next.swap(putNodePtr);

	_lru_tail->prev = _lru_tail->prev->next.get();

	_lru_index.insert(std::pair<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>>(_lru_tail->prev->key, (*(_lru_tail->prev))));
	_current_size += key.size() + value.size();
  return true;
}

bool SimpleLRU::DecreaseSizeIfNeeded(size_t oldNodeSize, size_t newNodeSize)
{
	while(_current_size - oldNodeSize + newNodeSize > _max_size)
	{
     DeleteFirst();

	 }

	return true;
}

bool SimpleLRU::Replace(lru_node &orig, const std::string &value)
{


		DecreaseSizeIfNeeded(orig.value.size(), value.size());
    _current_size += value.size();
		_current_size -= orig.value.size();

		orig.value = value;

		return MoveLast(orig);


}

bool SimpleLRU::MoveLast(lru_node &node)
{

	node.next->prev = node.prev;
	node.prev->next.swap(node.next);

	_lru_tail->prev->next.swap(node.next);
	_lru_tail->prev = &node;

	return true;

}

bool SimpleLRU::DeleteFirst()
{
	 return Delete(*(_lru_head->next.get()));
}



bool SimpleLRU::Delete(lru_node &rn)
{
	//special case - trying to delete fake nodes
	//return true;
	if(rn.key.length() == 0)
		return false;

  auto s(rn.key.size() + rn.value.size());
  std::unique_ptr<lru_node> tn;

	_lru_index.erase(rn.key);

	tn.swap(rn.prev->next);
	tn->next->prev=tn->prev;
	tn->prev->next.swap(tn->next);
	_current_size-=s;
	return true;
}


} // namespace Backend
} // namespace Afina
