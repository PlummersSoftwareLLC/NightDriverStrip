function defaultValidator<T>(o:any): o is T {
    return (o != undefined);
}

export function safeJsonParse<T>(guard: (o: any) => o is T = defaultValidator) {
  return (text: string): ParseResult<T> => {
    const parsed = JSON.parse(text)
    return guard(parsed) ? { parsed, hasError: false } : { hasError: true }
  }
}

export type ParseResult<T> =
  | { parsed: T; hasError: false; error?: undefined }
  | { parsed?: undefined; hasError: true; error?: unknown }