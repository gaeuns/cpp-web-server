import { useState, type FormEvent } from 'react'
import './App.css'

interface SearchResult {
  status: string
  query: string
  message: string
}

function App() {
  const [keyword, setKeyword] = useState('')
  const [loading, setLoading] = useState(false)
  const [result, setResult] = useState<SearchResult | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [searched, setSearched] = useState(false)

  const handleSearch = async (e: FormEvent) => {
    e.preventDefault()
    if (!keyword.trim()) return

    setLoading(true)
    setError(null)
    setSearched(true)

    try {
      const res = await fetch(`/api/search?keyword=${encodeURIComponent(keyword)}`)
      if (!res.ok) throw new Error(`서버 오류: ${res.status}`)
      const json: SearchResult = await res.json()
      setResult(json)
    } catch (err) {
      setError(err instanceof Error ? err.message : '알 수 없는 에러가 발생했습니다.')
      setResult(null)
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className={`page ${searched ? 'page--searched' : ''}`}>
      <header className="topbar">
        <span>Gmail</span>
        <span>이미지</span>
      </header>

      <main className="main">
        <div className="logo">
          <span className="logo-g">G</span>
          <span className="logo-o1">o</span>
          <span className="logo-o2">o</span>
          <span className="logo-g2">g</span>
          <span className="logo-l">l</span>
          <span className="logo-e">e</span>
        </div>

        <form className="search-form" onSubmit={handleSearch}>
          <div className="search-box">
            <svg className="search-icon" viewBox="0 0 24 24" width="20" height="20">
              <path
                fill="#9aa0a6"
                d="M15.5 14h-.79l-.28-.27a6.5 6.5 0 1 0-.7.7l.27.28v.79l5 5L20.49 19l-5-5zm-6 0A4.5 4.5 0 1 1 14 9.5 4.5 4.5 0 0 1 9.5 14z"
              />
            </svg>
            <input
              type="text"
              value={keyword}
              onChange={(e) => setKeyword(e.target.value)}
              placeholder="검색어를 입력하세요"
              autoFocus
            />
          </div>
          <div className="search-buttons">
            <button type="submit" className="btn">
              검색
            </button>
            <button
              type="button"
              className="btn"
              onClick={() => {
                setKeyword('')
                setResult(null)
                setError(null)
                setSearched(false)
              }}
            >
              지우기
            </button>
          </div>
        </form>

        {searched && (
          <section className="results">
            {loading && (
              <div className="loading">
                <div className="spinner" />
                <span>검색 중...</span>
              </div>
            )}

            {!loading && error && <div className="error">{error}</div>}

            {!loading && result && (
              <div className="result-card">
                <p className="result-meta">
                  '<strong>{result.query}</strong>'에 대한 검색 결과
                </p>
                <p className="result-message">{result.message}</p>
              </div>
            )}
          </section>
        )}
      </main>
    </div>
  )
}

export default App
