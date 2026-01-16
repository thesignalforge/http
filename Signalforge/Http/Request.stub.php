<?php
/**
 * Signalforge HTTP Extension
 * Request.stub.php - IDE stub for Request class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * High-performance HTTP Request handler implementing PSR-7 ServerRequestInterface.
 *
 * Provides direct HashTable access to PHP superglobals with zero-copy
 * strings and lazy evaluation. 10-50x faster than pure PHP implementations.
 *
 * @final
 * @implements \Psr\Http\Message\ServerRequestInterface
 */
final class Request implements \Psr\Http\Message\ServerRequestInterface
{
    /**
     * Private constructor - use Request::capture() instead.
     */
    private function __construct() {}

    /**
     * Create Request from current SAPI request context.
     *
     * Captures $_SERVER, $_GET, $_POST, $_COOKIE, and $_FILES superglobals.
     *
     * @return static
     * @throws \Exception if $_SERVER is not available
     */
    public static function capture(): static {}

    /* ============================================================================
     * PSR-7 MessageInterface
     * ============================================================================ */

    /**
     * Retrieves the HTTP protocol version as a string.
     *
     * @return string HTTP protocol version (e.g., "1.1", "1.0")
     */
    public function getProtocolVersion(): string {}

    /**
     * Return an instance with the specified HTTP protocol version.
     *
     * @param string $version HTTP protocol version (must be "1.0" or "1.1")
     * @return static
     * @throws \InvalidArgumentException for invalid protocol versions
     */
    public function withProtocolVersion(string $version): static {}

    /**
     * Retrieves all message header values.
     *
     * @return array<string, string[]> Returns an associative array of header names with array of values
     */
    public function getHeaders(): array {}

    /**
     * Checks if a header exists by the given case-insensitive name.
     *
     * @param string $name Case-insensitive header field name
     * @return bool Returns true if any header names match
     */
    public function hasHeader(string $name): bool {}

    /**
     * Retrieves a message header value by the given case-insensitive name.
     *
     * @param string $name Case-insensitive header field name
     * @return string[] Array of header values
     */
    public function getHeader(string $name): array {}

    /**
     * Retrieves a comma-separated string of the values for a single header.
     *
     * @param string $name Case-insensitive header field name
     * @return string Comma-separated string of header values
     */
    public function getHeaderLine(string $name): string {}

    /**
     * Return an instance with the provided value replacing the specified header.
     *
     * @param string $name Case-insensitive header field name
     * @param string|string[] $value Header value(s)
     * @return static
     * @throws \InvalidArgumentException for invalid header names or values
     */
    public function withHeader(string $name, $value): static {}

    /**
     * Return an instance with the specified header appended with the given value.
     *
     * @param string $name Case-insensitive header field name
     * @param string|string[] $value Header value(s)
     * @return static
     * @throws \InvalidArgumentException for invalid header names or values
     */
    public function withAddedHeader(string $name, $value): static {}

    /**
     * Return an instance without the specified header.
     *
     * @param string $name Case-insensitive header field name
     * @return static
     */
    public function withoutHeader(string $name): static {}

    /**
     * Gets the body of the message.
     *
     * @return \Psr\Http\Message\StreamInterface Returns the body as a stream
     */
    public function getBody(): \Psr\Http\Message\StreamInterface {}

    /**
     * Return an instance with the specified message body.
     *
     * @param \Psr\Http\Message\StreamInterface $body Body
     * @return static
     * @throws \InvalidArgumentException when the body is not valid
     */
    public function withBody(\Psr\Http\Message\StreamInterface $body): static {}

    /* ============================================================================
     * PSR-7 RequestInterface
     * ============================================================================ */

    /**
     * Retrieves the message's request target.
     *
     * @return string Request target
     */
    public function getRequestTarget(): string {}

    /**
     * Return an instance with the specific request-target.
     *
     * @param string $requestTarget Request target
     * @return static
     */
    public function withRequestTarget(string $requestTarget): static {}

    /**
     * Retrieves the HTTP method of the request.
     *
     * @return string Returns the request method
     */
    public function getMethod(): string {}

    /**
     * Return an instance with the provided HTTP method.
     *
     * @param string $method Case-sensitive method
     * @return static
     * @throws \InvalidArgumentException for invalid HTTP methods
     */
    public function withMethod(string $method): static {}

    /**
     * Retrieves the URI instance.
     *
     * @return \Psr\Http\Message\UriInterface URI instance
     */
    public function getUri(): \Psr\Http\Message\UriInterface {}

    /**
     * Returns an instance with the provided URI.
     *
     * @param string|\Psr\Http\Message\UriInterface $uri New request URI
     * @param bool $preserveHost Preserve the original state of the Host header
     * @return static
     */
    public function withUri($uri, bool $preserveHost = false): static {}

    /* ============================================================================
     * PSR-7 ServerRequestInterface
     * ============================================================================ */

    /**
     * Retrieve server parameters.
     *
     * @return array<string, mixed> Array of server parameters
     */
    public function getServerParams(): array {}

    /**
     * Retrieve cookies.
     *
     * @return array<string, string> Array of cookies
     */
    public function getCookieParams(): array {}

    /**
     * Return an instance with the specified cookies.
     *
     * @param array<string, string> $cookies Array of cookies
     * @return static
     */
    public function withCookieParams(array $cookies): static {}

    /**
     * Retrieve query string arguments.
     *
     * @return array<string, mixed> Array of query parameters
     */
    public function getQueryParams(): array {}

    /**
     * Return an instance with the specified query string arguments.
     *
     * @param array<string, mixed> $query Array of query string arguments
     * @return static
     */
    public function withQueryParams(array $query): static {}

    /**
     * Retrieve normalized file upload data.
     *
     * @return array<string, array> Array of uploaded files
     */
    public function getUploadedFiles(): array {}

    /**
     * Create a new instance with the specified uploaded files.
     *
     * @param array<string, array> $uploadedFiles Array of uploaded files
     * @return static
     * @throws \InvalidArgumentException if an invalid structure is provided
     */
    public function withUploadedFiles(array $uploadedFiles): static {}

    /**
     * Retrieve any parameters provided in the request body.
     *
     * @return null|array|object The deserialized body parameters, if any
     */
    public function getParsedBody() {}

    /**
     * Return an instance with the specified body parameters.
     *
     * @param null|array|object $data The deserialized body data
     * @return static
     * @throws \InvalidArgumentException if an unsupported argument type is provided
     */
    public function withParsedBody($data): static {}

    /**
     * Retrieve attributes derived from the request.
     *
     * @return array<string, mixed> Attributes
     */
    public function getAttributes(): array {}

    /**
     * Retrieve a single derived request attribute.
     *
     * @param string $name The attribute name
     * @param mixed $default Default value to return if the attribute does not exist
     * @return mixed
     */
    public function getAttribute(string $name, $default = null) {}

    /**
     * Return an instance with the specified derived request attribute.
     *
     * @param string $name The attribute name
     * @param mixed $value The value of the attribute
     * @return static
     */
    public function withAttribute(string $name, $value): static {}

    /**
     * Return an instance that removes the specified derived request attribute.
     *
     * @param string $name The attribute name
     * @return static
     */
    public function withoutAttribute(string $name): static {}
}

