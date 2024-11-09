package pkg

import "sync"

// HcSyncMap
// is an ordered sync map that
// keeps track of the key order
type HcSyncMap struct {
	data  map[string]any
	order []string
	mu    sync.RWMutex
}

//func NewOrderedSyncMap() *OrderedSyncMap {
//	return &OrderedSyncMap{
//		data:  make(map[string]interface{}),
//		order: make([]string, 0),
//	}
//}

func (m *HcSyncMap) ensure() {
	if m.data == nil {
		m.data = make(map[string]any)
	}
}

func (m *HcSyncMap) Store(key string, value interface{}) {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.ensure()

	if _, exists := m.data[key]; !exists {
		m.order = append(m.order, key)
	}

	m.data[key] = value
}

func (m *HcSyncMap) Load(key string) (any, bool) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	m.ensure()

	value, exists := m.data[key]

	return value, exists
}

func (m *HcSyncMap) Delete(key string) {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.ensure()

	if _, exists := m.data[key]; exists {
		delete(m.data, key)

		// remove key from order slice
		for i, k := range m.order {
			if k == key {
				m.order = append(m.order[:i], m.order[i+1:]...)
				break
			}
		}
	}
}

func (m *HcSyncMap) Range(f func(k string, v any) bool) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	m.ensure()

	for _, key := range m.order {
		value := m.data[key]
		if !f(key, value) {
			break
		}
	}
}
