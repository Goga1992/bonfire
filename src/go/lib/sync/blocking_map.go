package bon_sync

import (
	"sync"
)

type BlockingMap[K comparable, V any] struct {
	mtx  sync.RWMutex
	data map[K]V
}

type Tuple[K comparable, V any] struct {
	Key K
	Val V
}

func NewBlockingMap[K comparable, V any]() *BlockingMap[K, V] {
	return &BlockingMap[K, V]{
		data: make(map[K]V),
	}
}

func (m *BlockingMap[K, V]) Store(key K, val V) {
	m.mtx.Lock()
	defer m.mtx.Unlock()

	m.data[key] = val
}

func (m *BlockingMap[K, V]) Load(key K) (V, bool) {
	m.mtx.RLock()
	defer m.mtx.RUnlock()

	val, ok := m.data[key]
	return val, ok
}

func (m *BlockingMap[K, V]) Delete(key K) {
	m.mtx.Lock()
	defer m.mtx.Unlock()

	delete(m.data, key)
}

func (m *BlockingMap[K, V]) IterChannel(receiverDone chan bool) chan Tuple[K, V] {
	iterChannel := make(chan Tuple[K, V])

	go func() {
		m.mtx.RLock()
		defer m.mtx.RUnlock()

		for key, val := range m.data {
			iterChannel <- Tuple[K, V]{key, val}
		}

		close(iterChannel)

		<-receiverDone
	}()

	return iterChannel
}
