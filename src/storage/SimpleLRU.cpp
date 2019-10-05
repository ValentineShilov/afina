#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
	auto it = _lru_index.find(key);

	if (it == _lru_index.end())
	{
		return _Insert(key, value);
	}
	return _Replace(it->second.get().key, value);

}



bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
  if( ( value.size() + key.size() ) <= _max_size)
  {
		if(_lru_index.find(key)==_lru_index.end())
		{
			return _Insert(key,value);
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
  if( (value.size() + key.size() ) > _max_size)
	{
    return false;
	}
	if(_lru_index.find(key) != _lru_index.end())
	{
		return _Replace(key,value);
	}
	else
	{
    return false;
	}
}


bool SimpleLRU::Delete(const std::string &key)
 {
	auto it = _lru_index.find(key);
	if (it != _lru_index.end())
	{
	  return _Delete(it->second.get());
	}

	//not found - nothing to delete
	return false;

}



bool SimpleLRU::Get(const std::string &key, std::string &value)
{
	auto n = _lru_index.find(key);
	if(n != _lru_index.end())
	{
		value = n->second.get().value;
		return _MoveTail(n->second.get());
	}
	return false;
}

bool SimpleLRU::_Insert(const std::string &key, const std::string &value)
{
	_DecreaseSizeIfNeeded(0, key.size() + value.size());

	//some problem with make_unique on g++ 4.9 so constructor and new used
	auto putNodePtr = std::unique_ptr<lru_node>(new lru_node(key,value));

	if( _lru_tail != nullptr)
	{
			putNodePtr->prev = _lru_tail;
			_lru_tail->next.swap(putNodePtr);
			_lru_tail = _lru_tail->next.get();

	}
	else
	{
			//we have only head
			_lru_tail = putNodePtr.get();
			_lru_head.swap(putNodePtr);
	}

	_lru_index.insert(std::make_pair(key, std::ref(*_lru_tail)));

  return true;
}

bool SimpleLRU::_DecreaseSizeIfNeeded(size_t oldNodeSize, size_t newNodeSize)
{
	while(_current_size - oldNodeSize + newNodeSize > _max_size)
	{
     _DeleteHead();
	 }
	_current_size += newNodeSize - oldNodeSize;
	return true;
}

bool SimpleLRU::_Replace(const std::string &key, const std::string &nn)
{

	auto onp  = _lru_index.find(key);
	if(onp !=_lru_index.end())
	{
		lru_node&  on  = _lru_index.find(key)->second.get();
		_DecreaseSizeIfNeeded(on.value.size(), nn.size());
		on.value = nn;
		return _MoveTail(on);
	}
	return false;
}

bool SimpleLRU::_MoveTail(lru_node &node)
{
  //moving tail to the tail node - do nothing
	if( &node == _lru_tail )
	{
    return true;
	}
	//if head
  if(&node == _lru_head.get())
	{
    _lru_head.swap(node.next);

    _lru_head->prev = nullptr;
  }
  else
	{
    node.next->prev = node.prev;
    node.prev->next.swap(node.next);
  }

   _lru_tail->next.swap(node.next);
   node.prev = _lru_tail;
   _lru_tail = &node;

   return true;
}

bool SimpleLRU::_DeleteHead()
{
	  std::unique_ptr<lru_node> tn;
		_lru_index.erase(_lru_head->key);
		_current_size -= _lru_head->key.size();
		_current_size -= _lru_head->value.size();

		tn.swap(_lru_head);
    _lru_head.swap(tn->next);
    _lru_head->prev = nullptr;
		return true;

}

bool SimpleLRU::_Delete(lru_node &rn)
{
	//pointer is  used to destruct nodes
   std::unique_ptr<lru_node> tn;

	if(&rn == _lru_head.get())
  {
   	return _DeleteHead();

  }

	_lru_index.erase(rn.key);
	_current_size -= rn.key.size();
	_current_size -= rn.value.size();

	if(&rn == _lru_tail)
  {

		_MoveTail(*(_lru_tail->prev));

  }
  else
	{
		tn.swap(rn.prev->next);
		tn->prev->next.swap(tn->next);
		tn->next->prev=tn->prev;
	}

 return true;
}


} // namespace Backend
} // namespace Afina
