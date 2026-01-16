<?php
/**
 * Signalforge HTTP Extension - Basic Usage Examples
 *
 * This file demonstrates the core functionality of the signalforge_http
 * extension. All classes live in the \Signalforge\NativeHttp namespace.
 */

declare(strict_types=1);

use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Response;
use Signalforge\NativeHttp\Stream;
use Signalforge\NativeHttp\Uri;
use Signalforge\NativeHttp\UploadedFile;

// Verify extension is loaded
if (!extension_loaded('signalforge_http')) {
    die("Error: signalforge_http extension is not loaded\n");
}

echo "=== Signalforge HTTP Extension Examples ===\n\n";

// -----------------------------------------------------------------------------
// 1. Response Creation
// -----------------------------------------------------------------------------
echo "1. Response Creation\n";
echo str_repeat('-', 50) . "\n";

// Basic response
$response = Response::create(200, ['Content-Type' => 'application/json'], '{"status":"ok"}');
echo "Status: " . $response->getStatusCode() . " " . $response->getReasonPhrase() . "\n";
echo "Content-Type: " . $response->getHeaderLine('Content-Type') . "\n";
echo "Body: " . (string)$response->getBody() . "\n\n";

// Factory methods
$jsonResponse = Response::json(['message' => 'Hello, World!']);
echo "JSON Response: " . (string)$jsonResponse->getBody() . "\n";

$textResponse = Response::text('Plain text content');
echo "Text Response: " . (string)$textResponse->getBody() . "\n";

$htmlResponse = Response::html('<h1>Hello</h1>');
echo "HTML Content-Type: " . $htmlResponse->getHeaderLine('Content-Type') . "\n";

$redirect = Response::redirect('https://example.com/new-location', 302);
echo "Redirect Location: " . $redirect->getHeaderLine('Location') . "\n\n";

// -----------------------------------------------------------------------------
// 2. Stream Handling
// -----------------------------------------------------------------------------
echo "2. Stream Handling\n";
echo str_repeat('-', 50) . "\n";

// Create stream from string
$stream = Stream::fromString("Hello, this is stream content!");
echo "Stream size: " . $stream->getSize() . " bytes\n";
echo "Stream content: " . $stream->getContents() . "\n";
echo "Is readable: " . ($stream->isReadable() ? 'yes' : 'no') . "\n";
echo "Is seekable: " . ($stream->isSeekable() ? 'yes' : 'no') . "\n\n";

// Rewind and read again
$stream->rewind();
echo "After rewind: " . $stream->read(5) . "...\n\n";

// -----------------------------------------------------------------------------
// 3. URI Manipulation
// -----------------------------------------------------------------------------
echo "3. URI Manipulation\n";
echo str_repeat('-', 50) . "\n";

$uri = Uri::fromString('https://user:pass@example.com:8080/path?query=value#fragment');
echo "Scheme: " . $uri->getScheme() . "\n";
echo "Host: " . $uri->getHost() . "\n";
echo "Port: " . ($uri->getPort() ?? 'default') . "\n";
echo "Path: " . $uri->getPath() . "\n";
echo "Query: " . $uri->getQuery() . "\n";
echo "Fragment: " . $uri->getFragment() . "\n";
echo "Authority: " . $uri->getAuthority() . "\n";
echo "Full URI: " . (string)$uri . "\n\n";

// Immutable modifications
$newUri = $uri->withScheme('http')
    ->withHost('api.example.com')
    ->withPath('/v2/users')
    ->withQuery('page=1&limit=10');
echo "Modified URI: " . (string)$newUri . "\n\n";

// -----------------------------------------------------------------------------
// 4. Request Creation (Server Request)
// -----------------------------------------------------------------------------
echo "4. Request Creation\n";
echo str_repeat('-', 50) . "\n";

// Create from globals using capture() (simulated superglobals)
$_SERVER['REQUEST_METHOD'] = 'POST';
$_SERVER['REQUEST_URI'] = '/api/users';
$_SERVER['HTTP_CONTENT_TYPE'] = 'application/json';
$_SERVER['HTTP_AUTHORIZATION'] = 'Bearer token123';

$request = Request::capture();
echo "Method: " . $request->getMethod() . "\n";
echo "Request Target: " . $request->getRequestTarget() . "\n";
echo "Has Authorization: " . ($request->hasHeader('Authorization') ? 'yes' : 'no') . "\n";
echo "Authorization: " . $request->getHeaderLine('Authorization') . "\n\n";

// Immutable modifications
$newRequest = $request->withMethod('PUT')
    ->withHeader('X-Custom-Header', 'custom-value');
echo "Modified method: " . $newRequest->getMethod() . "\n";
echo "Custom header: " . $newRequest->getHeaderLine('X-Custom-Header') . "\n\n";

// -----------------------------------------------------------------------------
// 5. PSR-18 HTTP Client (if enabled)
// -----------------------------------------------------------------------------
if (class_exists('\\Signalforge\\NativeHttp\\Client')) {
    echo "5. PSR-18 HTTP Client\n";
    echo str_repeat('-', 50) . "\n";

    try {
        $client = new \Signalforge\NativeHttp\Client();

        // Create a PSR-7 request using Request::create(method, uri)
        $clientRequest = Request::create('GET', 'https://httpbin.org/get');

        echo "Making HTTP request to httpbin.org...\n";
        $response = $client->sendRequest($clientRequest);

        echo "Response status: " . $response->getStatusCode() . "\n";
        echo "Response size: " . $response->getBody()->getSize() . " bytes\n\n";
    } catch (\Exception $e) {
        echo "Client example skipped (network unavailable): " . $e->getMessage() . "\n\n";
    }
} else {
    echo "5. PSR-18 HTTP Client\n";
    echo str_repeat('-', 50) . "\n";
    echo "Client not available (build with --enable-signalforge-http-client)\n\n";
}

// -----------------------------------------------------------------------------
// 6. Header Case-Insensitivity
// -----------------------------------------------------------------------------
echo "6. Header Case-Insensitivity (PSR-7 Compliance)\n";
echo str_repeat('-', 50) . "\n";

$response = Response::create(200, [
    'Content-Type' => 'application/json',
    'X-Custom-Header' => 'value1'
]);

echo "hasHeader('content-type'): " . ($response->hasHeader('content-type') ? 'yes' : 'no') . "\n";
echo "hasHeader('CONTENT-TYPE'): " . ($response->hasHeader('CONTENT-TYPE') ? 'yes' : 'no') . "\n";
echo "getHeaderLine('x-custom-header'): " . $response->getHeaderLine('x-custom-header') . "\n\n";

// -----------------------------------------------------------------------------
// 7. Immutability Demonstration
// -----------------------------------------------------------------------------
echo "7. Immutability (PSR-7 Compliance)\n";
echo str_repeat('-', 50) . "\n";

$original = Response::create(200);
$modified = $original->withStatus(404, 'Not Found')
    ->withHeader('X-Error', 'Resource not found');

echo "Original status: " . $original->getStatusCode() . "\n";
echo "Modified status: " . $modified->getStatusCode() . "\n";
echo "Original has X-Error: " . ($original->hasHeader('X-Error') ? 'yes' : 'no') . "\n";
echo "Modified has X-Error: " . ($modified->hasHeader('X-Error') ? 'yes' : 'no') . "\n\n";

echo "=== All examples completed successfully! ===\n";
