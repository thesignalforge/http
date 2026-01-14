# Signalforge HTTP Extension

[![CI](https://github.com/signalforge/http/actions/workflows/ci.yml/badge.svg)](https://github.com/signalforge/http/actions/workflows/ci.yml)
[![PHP 8.3+](https://img.shields.io/badge/PHP-8.3%2B-blue.svg)](https://php.net)

A native PHP extension implementing high-performance PSR-7 HTTP Request and Response classes with zero-copy operations and direct superglobal access.

## What's Different

- **Native C implementation** - all HTTP operations run in native code
- **Zero-copy string streams** - reference strings directly without data duplication
- **Direct HashTable access** - bypass PHP arrays for superglobal data
- **Lazy evaluation** - parse data only when accessed
- **Immutable objects** - all `with*()` methods return new instances
- **Memory efficient** - proper reference counting and cleanup
- **PSR-7 compliant** - implements `ServerRequestInterface`, `ResponseInterface`, `StreamInterface`, `UriInterface`, and `UploadedFileInterface`
- **Optimized for php-fpm** - designed for FastCGI environments
- **No dependencies** - pure C extension with no external libraries

## Why C?

HTTP request/response handling is invoked on nearly every request, often hundreds of times. Moving HTTP operations to native code provides:

- **Direct superglobal access** - bypass PHP's array layer for $_SERVER, $_GET, $_POST, $_COOKIE, $_FILES
- **Zero-copy string operations** - reference string data directly without duplication
- **Native hash tables** - efficient storage and lookup for headers and parameters
- **Reduced overhead** - minimal PHP engine interaction during data access
- **Memory efficiency** - proper reference counting and cleanup
- **Lazy evaluation** - parse JSON/form data only when requested
- **Immutable operations** - efficient object cloning with shared data structures

## Features

- **Full PSR-7 Compliance**: Implements `ServerRequestInterface`, `ResponseInterface`, `StreamInterface`, `UriInterface`, and `UploadedFileInterface`
- **Zero Dependencies**: Pure C extension with no external libraries
- **Hyper-Performance**: Direct HashTable access, zero-copy operations, lazy evaluation
- **Immutable Objects**: All `with*()` methods return new instances
- **Memory Efficient**: Proper reference counting and cleanup

## Streamforge Proxy Integration

The extension integrates seamlessly with the [Streamforge](../streamforge) FastCGI proxy for high-performance file upload handling. When Streamforge is deployed between nginx and php-fpm, it offloads file upload I/O from PHP workers, dramatically improving throughput for upload-heavy applications.

### How It Works

```
WITHOUT STREAMFORGE:
┌────────┐     ┌─────────┐     ┌───────────┐
│ nginx  │────▶│ php-fpm │────▶│  $_FILES  │
└────────┘     └─────────┘     └───────────┘
                    │
            Worker blocked during
            entire upload duration

WITH STREAMFORGE:
┌────────┐     ┌─────────────┐     ┌─────────┐     ┌─────────────────────┐
│ nginx  │────▶│ streamforge │────▶│ php-fpm │────▶│ HTTP_X_UPLOAD_* hdrs│
└────────┘     └─────────────┘     └─────────┘     └─────────────────────┘
                     │
              Files written to disk
              before PHP starts
```

### Benefits

| Scenario | Standard PHP | With Streamforge |
|----------|--------------|------------------|
| 500MB upload over slow connection | Worker blocked ~30s | Worker engaged ~5ms |
| Memory per upload | Up to 500MB buffered | ~100KB proxy buffers |
| 20 concurrent uploads, 10 workers | Site unresponsive | No impact on other requests |

### Transparent Integration

The extension automatically detects Streamforge and reads uploads from the appropriate source. **Your application code remains unchanged**:

```php
// Works identically with or without Streamforge
$request = Request::capture();
$files = $request->getUploadedFiles();

foreach ($files as $name => $file) {
    $file->getClientFilename();  // "document.pdf"
    $file->getSize();            // 52428800
    $file->moveTo('/storage/docs/document.pdf');
}
```

### Detection API

Check if Streamforge is handling the current request:

```php
use Signalforge\NativeHttp\Request;

// Static method
if (Request::isStreamforgeEnabled()) {
    // Streamforge is proxying this request
}

// Or check $_SERVER directly
if (isset($_SERVER['HTTP_X_STREAMFORGE'])) {
    // Streamforge marker present
}

// Check for processed uploads
if (isset($_SERVER['HTTP_X_UPLOAD_FILE_COUNT'])) {
    $count = (int) $_SERVER['HTTP_X_UPLOAD_FILE_COUNT'];
    // Streamforge handled $count file uploads
}
```

### Protocol

When Streamforge handles multipart uploads, it:

1. Parses the multipart body and writes files to disk
2. Adds metadata headers to the FastCGI request:
   - `HTTP_X_STREAMFORGE=1` - Proxy marker
   - `HTTP_X_UPLOAD_FILE_COUNT=N` - Number of uploaded files
   - `HTTP_X_UPLOAD_0_NAME` - Form field name
   - `HTTP_X_UPLOAD_0_FILENAME` - Original client filename
   - `HTTP_X_UPLOAD_0_PATH` - Temp file path on disk
   - `HTTP_X_UPLOAD_0_SIZE` - File size in bytes
   - `HTTP_X_UPLOAD_0_TYPE` - MIME type
3. Sends only form fields (not file content) to PHP-FPM

The extension reads these headers and creates `UploadedFile` objects that work identically to standard PHP uploads.

### Cleanup

Temp files are automatically cleaned up:
- **On `moveTo()`**: File is moved, no cleanup needed
- **On request end**: Unmoved temp files are deleted by the extension's RSHUTDOWN handler

This prevents disk space leaks even if application code doesn't handle all uploaded files.

### Deployment

See the [Streamforge documentation](../streamforge/README.md) for deployment instructions. Basic setup:

```bash
# Start Streamforge between nginx and php-fpm
streamforge -l 0.0.0.0:9001 -u /var/run/php-fpm.sock -d /tmp/uploads

# Configure nginx to send requests to Streamforge
# fastcgi_pass 127.0.0.1:9001;
```

## Requirements

- PHP 8.3, 8.4, or 8.5
- Linux or macOS (tested on x86_64 and ARM64)
- php-fpm recommended (works in CLI for testing)

## Building

### Docker (Recommended)

No need to install PHP dev headers on your host:

```bash
cd http

# Build Docker image with extension
make docker-build

# Run tests
make docker-test

# Test all PHP versions (8.3, 8.4, 8.5)
make ci-test-all

# Run example
make docker-example
```

### Host Installation

```bash
cd http
phpize
./configure --enable-signalforge_http
make
make test
sudo make install
```

Then add `extension=signalforge_http.so` to your php.ini.

## Usage

### Request

```php
<?php

use Signalforge\NativeHttp\Request;

// Capture the current request
$request = Request::capture();

// HTTP Method & URI
$method = $request->getMethod();                    // "POST"
$target = $request->getRequestTarget();            // "/users/123?include=profile"
$path = $request->getUri();                         // "/users/123?include=profile"

// Headers (case-insensitive)
$contentType = $request->getHeader('Content-Type'); // ['application/json']
$contentTypeLine = $request->getHeaderLine('Content-Type'); // "application/json"
$hasAuth = $request->hasHeader('Authorization');   // true/false
$allHeaders = $request->getHeaders();              // ['content-type' => ['application/json']]

// Parameters
$queryParams = $request->getQueryParams();         // $_GET as array
$parsedBody = $request->getParsedBody();           // JSON/form data (lazy parsed)

// Body access
$bodyStream = $request->getBody();                 // StreamInterface
$rawBody = (string) $request->getBody();           // Raw body string

// Server & environment
$serverParams = $request->getServerParams();       // $_SERVER
$userAgent = $serverParams['HTTP_USER_AGENT'];

// Cookies
$cookies = $request->getCookieParams();            // $_COOKIE as array
$sessionId = $cookies['session_id'];

// Uploaded files
$files = $request->getUploadedFiles();             // Normalized file structure
if (isset($files['avatar'])) {
    $filename = $files['avatar']->getClientFilename();
    $files['avatar']->moveTo('/uploads/' . $filename);
}

// Attributes (middleware data)
$request = $request->withAttribute('user_id', 123);
$userId = $request->getAttribute('user_id');       // 123
$userId = $request->getAttribute('missing', 'default'); // 'default'

// Immutable modifications
$newRequest = $request
    ->withMethod('PUT')
    ->withHeader('X-API-Key', 'secret')
    ->withQueryParams(['limit' => 10])
    ->withParsedBody(['name' => 'John']);

// Original request unchanged
assert($request->getMethod() === 'POST');
assert($newRequest->getMethod() === 'PUT');
```

### Response

```php
<?php

use Signalforge\NativeHttp\Response;
use Signalforge\NativeHttp\Stream;

// Factory methods
$response = Response::create(200, ['Content-Type' => 'application/json']);
$response = Response::json(['users' => ['id' => 1, 'name' => 'John']], 200);
$response = Response::text('Hello World', 200);
$response = Response::html('<h1>Welcome</h1>', 200);
$response = Response::redirect('/login', 302);

// Status management
$statusCode = $response->getStatusCode();          // 200
$reasonPhrase = $response->getReasonPhrase();      // "OK"
$response = $response->withStatus(404, 'Not Found');

// Header management (case-insensitive)
$response = $response->withHeader('Content-Type', 'application/json');
$response = $response->withAddedHeader('Cache-Control', 'no-cache');
$response = $response->withAddedHeader('Cache-Control', 'private');
$hasHeader = $response->hasHeader('Content-Type'); // true
$headerValue = $response->getHeader('Content-Type'); // ['application/json']
$headerLine = $response->getHeaderLine('Content-Type'); // "application/json"
$allHeaders = $response->getHeaders();

// Body management
$stream = Stream::fromString('{"message": "Hello"}');
$response = $response->withBody($stream);
$bodyStream = $response->getBody();

// Output
$response->send();                                  // Send headers + body
$response->sendHeaders();                           // Send only headers
$response->sendBody();                              // Send only body

// Serialization
$message = (string) $response;                      // Full HTTP message
```

### Stream

```php
<?php

use Signalforge\NativeHttp\Stream;

// Factory methods
$stream = Stream::fromString('Hello World');        // TRUE zero-copy string reference
$stream = Stream::fromResource(fopen('file.txt', 'r')); // From PHP resource
$stream = Stream::fromFile('/path/to/file', 'r');  // From file path

// Reading operations
$data = $stream->read(5);                          // Read 5 bytes: "Hello"
$remaining = $stream->getContents();               // Get rest: " World"
$stream->rewind();                                 // Reset to beginning
$all = (string) $stream;                           // Get entire contents

// Writing operations (use file or resource streams for writing)
$writableStream = Stream::fromFile('/tmp/output.txt', 'w+');
$bytesWritten = $writableStream->write('Hello');   // Write data
$writableStream->write(' World');                  // Append more

// Seeking operations
$stream->seek(6);                                  // Seek to position 6
$position = $stream->tell();                       // Get current position: 6
$stream->rewind();                                 // Reset to beginning

// Stream capabilities
$isReadable = $stream->isReadable();               // Check if can read
$isWritable = $stream->isWritable();               // Check if can write
$isSeekable = $stream->isSeekable();               // Check if supports seeking
$atEnd = $stream->eof();                           // Check if at end

// Metadata and size
$size = $stream->getSize();                        // Size in bytes (or null)
$metadata = $stream->getMetadata();                // All metadata
$uri = $stream->getMetadata('uri');                // Specific metadata key

// Resource management
$underlying = $stream->detach();                   // Detach PHP resource
$stream->close();                                  // Close stream and free resources
```

### Uri

```php
<?php

use Signalforge\NativeHttp\Uri;

// Parse a URI string
$uri = Uri::fromString('https://user:pass@example.com:8080/path?query=value#fragment');

// Access components (PSR-7 UriInterface)
$scheme = $uri->getScheme();       // "https"
$userInfo = $uri->getUserInfo();   // "user:pass"
$host = $uri->getHost();           // "example.com"
$port = $uri->getPort();           // 8080 (null if standard port for scheme)
$path = $uri->getPath();           // "/path"
$query = $uri->getQuery();         // "query=value"
$fragment = $uri->getFragment();   // "fragment"
$authority = $uri->getAuthority(); // "user:pass@example.com:8080"

// Serialize to string
$uriString = (string) $uri;        // "https://user:pass@example.com:8080/path?query=value#fragment"

// Immutable modifications
$newUri = $uri
    ->withScheme('http')
    ->withHost('api.example.com')
    ->withPort(null)               // Remove explicit port
    ->withPath('/v2/users')
    ->withQuery('limit=10')
    ->withFragment('');

// Original URI unchanged
assert($uri->getHost() === 'example.com');
assert($newUri->getHost() === 'api.example.com');

// Standard ports are normalized to null
$httpsUri = Uri::fromString('https://example.com:443/path');
$port = $httpsUri->getPort();      // null (443 is standard for https)
```

### UploadedFile

```php
<?php

use Signalforge\NativeHttp\Request;

// Get uploaded files from request
$request = Request::capture();
$files = $request->getUploadedFiles();

// Single file upload
if (isset($files['avatar'])) {
    $file = $files['avatar'];

    // File properties
    $size = $file->getSize();                       // Size in bytes
    $error = $file->getError();                     // UPLOAD_ERR_* constant
    $clientName = $file->getClientFilename();       // Original filename
    $mimeType = $file->getClientMediaType();        // MIME type

    // Move file to permanent location
    $targetPath = '/uploads/avatars/' . uniqid() . '_' . $clientName;
    $file->moveTo($targetPath);

    // Note: moveTo() can only be called once per UploadedFile
}

// Multiple file upload
if (isset($files['photos'])) {
    foreach ($files['photos'] as $photo) {
        if ($photo->getError() === UPLOAD_ERR_OK) {
            $filename = $photo->getClientFilename();
            $photo->moveTo('/uploads/photos/' . $filename);
        }
    }
}

// Stream access (alternative to moveTo)
$stream = $file->getStream();
$content = $stream->getContents();
```

### Advanced Patterns

```php
<?php

use Signalforge\NativeHttp\{Request, Response, Stream};

// Middleware-style request processing
function authenticate(Request $request): Request
{
    $token = $request->getHeaderLine('Authorization');
    $userId = validateToken($token);

    return $request->withAttribute('user_id', $userId);
}

function validateJson(Request $request): Request
{
    $contentType = $request->getHeaderLine('Content-Type');
    if (!str_contains($contentType, 'application/json')) {
        throw new InvalidArgumentException('JSON content type required');
    }

    return $request;
}

// Request processing pipeline
$request = Request::capture();
$request = authenticate($request);
$request = validateJson($request);

// JSON API response
$data = ['users' => getUsers($request->getAttribute('user_id'))];
$response = Response::json($data, 200);

// CORS headers
$response = $response
    ->withHeader('Access-Control-Allow-Origin', '*')
    ->withHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE')
    ->withHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');

// Conditional response
if ($request->hasHeader('If-None-Match')) {
    $etag = $request->getHeaderLine('If-None-Match');
    if ($etag === generateEtag($data)) {
        $response = $response->withStatus(304); // Not Modified
    }
}

$response->send();
```

## API Reference

### Request

#### Factory Methods

```php
Request::capture(): ServerRequestInterface  // Capture current request from superglobals
```

#### PSR-7 MessageInterface Methods

```php
getProtocolVersion(): string                           // Get HTTP protocol version (always "1.1" in FastCGI)
withProtocolVersion(string $version): static          // Return new instance with protocol version

getHeaders(): array                                   // Get all headers as lowercase key => array values
hasHeader(string $name): bool                         // Check if header exists (case-insensitive)
getHeader(string $name): array                        // Get header values array
getHeaderLine(string $name): string                    // Get header values as comma-separated string

withHeader(string $name, string|array $value): static  // Replace header (case-insensitive)
withAddedHeader(string $name, string|array $value): static // Add to existing header
withoutHeader(string $name): static                   // Remove header

getBody(): StreamInterface                            // Get message body stream
withBody(StreamInterface $body): static               // Replace body stream
```

#### PSR-7 RequestInterface Methods

```php
getRequestTarget(): string                            // Get request target (path + query)
withRequestTarget(string $target): static             // Set request target

getMethod(): string                                   // Get HTTP method
withMethod(string $method): static                    // Set HTTP method

getUri(): string                                      // Get URI as string
withUri(string|UriInterface $uri, bool $preserveHost = false): static // Set URI
```

#### PSR-7 ServerRequestInterface Methods

```php
getServerParams(): array                              // Get $_SERVER parameters
getCookieParams(): array                              // Get $_COOKIE parameters
withCookieParams(array $cookies): static              // Replace cookies
getQueryParams(): array                               // Get $_GET parameters
withQueryParams(array $query): static                 // Replace query parameters

getUploadedFiles(): array                             // Get uploaded files structure
withUploadedFiles(array $files): static               // Replace uploaded files

getParsedBody(): array|object|null                    // Get parsed body (JSON/form data)
withParsedBody(array|object|null $data): static       // Set parsed body

getAttributes(): array                                // Get request attributes
getAttribute(string $name, mixed $default = null)     // Get single attribute
withAttribute(string $name, mixed $value): static     // Add/replace attribute
withoutAttribute(string $name): static                // Remove attribute
```

### Response

#### Factory Methods

```php
Response::create(int $status = 200, array $headers = [], mixed $body = null): static
Response::json(mixed $data, int $status = 200): static
Response::text(string $text, int $status = 200): static
Response::html(string $html, int $status = 200): static
Response::redirect(string $url, int $status = 302): static
```

#### PSR-7 MessageInterface Methods

```php
getProtocolVersion(): string                          // Get HTTP protocol version
withProtocolVersion(string $version): static          // Set protocol version

getHeaders(): array                                   // Get all headers
hasHeader(string $name): bool                         // Check header exists
getHeader(string $name): array                        // Get header values
getHeaderLine(string $name): string                    // Get comma-separated header

withHeader(string $name, string|array $value): static  // Replace header
withAddedHeader(string $name, string|array $value): static // Add header value
withoutHeader(string $name): static                   // Remove header

getBody(): StreamInterface                            // Get body stream
withBody(StreamInterface $body): static               // Replace body stream
```

#### PSR-7 ResponseInterface Methods

```php
getStatusCode(): int                                  // Get HTTP status code
withStatus(int $code, string $reason = ''): static    // Set status code and reason
getReasonPhrase(): string                             // Get reason phrase
```

#### Output Methods

```php
send(): void                                           // Send response (headers + body)
sendHeaders(): void                                    // Send only headers
sendBody(): void                                        // Send only body
__toString(): string                                    // Serialize to HTTP message
```

### Stream

#### Factory Methods

```php
Stream::fromString(string $string): static             // Create from string (zero-copy)
Stream::fromResource(resource $resource): static       // Create from PHP stream resource
Stream::fromFile(string $path, string $mode = 'r'): static // Create from file
```

#### PSR-7 StreamInterface Methods

```php
read(int $length): string                              // Read data from stream
getContents(): string                                  // Get remaining contents
write(string $string): int                             // Write data to stream

seek(int $offset, int $whence = SEEK_SET): void        // Seek to position
tell(): int                                            // Get current position
rewind(): void                                         // Seek to beginning

eof(): bool                                            // Check if at end of stream
isReadable(): bool                                     // Check if stream is readable
isWritable(): bool                                     // Check if stream is writable
isSeekable(): bool                                     // Check if stream supports seeking

getSize(): ?int                                        // Get stream size (if known)
getMetadata(?string $key = null): mixed                // Get stream metadata

close(): void                                          // Close stream and free resources
detach(): resource|null                                // Detach underlying resource

__toString(): string                                   // Get entire stream contents
```

### UploadedFile

#### PSR-7 UploadedFileInterface Methods

```php
getStream(): StreamInterface                           // Get file contents as stream
moveTo(string $targetPath): void                       // Move file to new location
getSize(): ?int                                        // Get file size in bytes
getError(): int                                        // Get upload error code (UPLOAD_ERR_*)
getClientFilename(): ?string                           // Get original client filename
getClientMediaType(): ?string                          // Get client-provided MIME type
```

### Uri

#### Factory Methods

```php
Uri::fromString(string $uri): UriInterface             // Parse URI string (RFC 3986 compliant)
```

#### PSR-7 UriInterface Methods

```php
getScheme(): string                                    // Get URI scheme (http, https, etc.)
getAuthority(): string                                 // Get authority (userinfo@host:port)
getUserInfo(): string                                  // Get user info (user:pass)
getHost(): string                                      // Get host (lowercase)
getPort(): ?int                                        // Get port (null if standard for scheme)
getPath(): string                                      // Get path component
getQuery(): string                                     // Get query string (without ?)
getFragment(): string                                  // Get fragment (without #)

withScheme(string $scheme): UriInterface               // Return new instance with scheme
withUserInfo(string $user, ?string $pass = null): UriInterface // Set user info
withHost(string $host): UriInterface                   // Set host
withPort(?int $port): UriInterface                     // Set port (null to remove)
withPath(string $path): UriInterface                   // Set path
withQuery(string $query): UriInterface                 // Set query string
withFragment(string $fragment): UriInterface           // Set fragment

__toString(): string                                   // Serialize to URI string
```

## Performance

The extension provides significant performance improvements over userland PSR-7 implementations through native C code, direct superglobal access, and zero-copy operations. Benchmarks comparing against other PSR-7 implementations can be found in the [http-php repository](https://github.com/signalforge/http-php).

### Key Optimizations

- **Direct superglobal access** - bypasses PHP's array layer for $_SERVER, $_GET, $_POST, $_COOKIE, $_FILES
- **Zero-copy string streams** - reference strings directly without data duplication
- **Native hash tables** - efficient storage and lookup for headers and parameters
- **Lazy evaluation** - parse JSON/form data only when accessed
- **Immutable operations** - efficient object cloning with shared data structures
- **Memory efficient** - proper reference counting and cleanup

## How It Works

### Request Capture Process

1. **Direct superglobal access** - References $_SERVER, $_GET, $_POST, $_COOKIE, $_FILES directly
2. **Lazy header parsing** - Headers parsed only when `getHeaders()` is called
3. **JSON caching** - Parsed JSON bodies cached to avoid re-parsing
4. **Immutable cloning** - `with*()` methods create efficient clones with shared data

### Stream Operations

- **String streams**: TRUE zero-copy references to existing strings (no data duplication)
- **Resource streams**: Efficient `php_stream_copy_to_mem()` for large data
- **Lazy loading**: Stream contents read only when accessed
- **Position tracking**: Efficient position management for seekable streams

### Memory Management

- **Reference counting**: Proper Zend reference counting throughout
- **Object pooling**: Reuses memory structures where possible
- **Automatic cleanup**: Destructors handle resource cleanup
- **Leak prevention**: All allocations properly tracked and freed

## Structure

```
http/
├── config.m4                    # Build configuration
├── signalforge_http.c           # PHP class implementations
├── php_signalforge_http.h       # Main header
├── src/
│   ├── request.c/h              # Request class implementation
│   ├── response.c/h             # Response class implementation
│   ├── stream.c/h               # Stream class implementation
│   ├── uri.c/h                  # Uri class implementation
│   ├── uploadedfile.c/h         # UploadedFile class implementation
│   ├── psr7_interfaces.c/h      # PSR-7 interface definitions
├── Signalforge/Http/            # IDE stubs
├── examples/                    # Usage examples
├── tests/                       # phpt test files (97 tests)
└── Dockerfile                   # Docker build environment
```

## Testing

```bash
make test
```

Or run specific tests:

```bash
docker run --rm signalforge-http php /opt/run-tests.php tests/001_basic.phpt
```

## Memory Leak Detection

```bash
# Docker-based Valgrind (recommended)
make valgrind-docker

# Local Valgrind (requires valgrind installed)
make valgrind-test
```

## Thread Safety

The extension supports ZTS (Zend Thread Safety) builds. Each request gets isolated instances, and all operations are thread-safe.

## Exception Handling

- `InvalidArgumentException` - Invalid parameters or malformed data
- `RuntimeException` - Stream operations, file access errors
- Standard PHP exceptions for JSON parsing errors

## Related

- [signalforge/http-php](https://github.com/signalforge/http-php) - PHP Composer package with PSR-7/PSR-17 wrappers
- [PSR-7](https://www.php-fig.org/psr/psr-7/) - HTTP Message Interface specification
- [PSR-17](https://www.php-fig.org/psr/psr-17/) - HTTP Factories specification

## License

MIT License

