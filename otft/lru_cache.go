package main

import (
	"container/list"
	"sync"
)

type CacheItem struct {
	Key   string
	Value string
}

type LRUCache struct {
	capacity   int
	items      map[string]*list.Element
	order      *list.List
	onEvicted  func(key, value string)
	mu         sync.Mutex
	protected map[string]bool
}

func NewLRUCache(capacity int, onEvicted func(key, value string)) *LRUCache {
	return &LRUCache{
		capacity:  capacity,
		items:     make(map[string]*list.Element),
		order:     list.New(),
		onEvicted: onEvicted,
		protected: make(map[string]bool),
	}
}

func (c *LRUCache) Get(key string) (string, bool) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if elem, found := c.items[key]; found {
		c.order.MoveToFront(elem)
		return elem.Value.(*CacheItem).Value, true
	}
	return "", false
}

func (c *LRUCache) Put(key, value string) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if elem, found := c.items[key]; found {
		c.order.MoveToFront(elem)
		elem.Value.(*CacheItem).Value = value
		return
	}

	if c.order.Len() == c.capacity {
		lastElem := c.order.Back()
		if lastElem != nil {
			lastItem := lastElem.Value.(*CacheItem)
			if c.onEvicted != nil && !c.protected[lastItem.Key]{
				c.onEvicted(lastItem.Key, lastItem.Value)
			}
			delete(c.items, lastItem.Key)
			c.order.Remove(lastElem)
		}
	}

	newItem := &CacheItem{Key: key, Value: value}
	newElem := c.order.PushFront(newItem)
	c.items[key] = newElem
}

func (c *LRUCache) Protect(key string) {
	c.mu.Lock()
	defer c.mu.Unlock()

	c.protected[key] = true
}

func (c *LRUCache) Unprotect(key string) {
	c.mu.Lock()
	defer c.mu.Unlock()

	delete(c.protected, key)
}