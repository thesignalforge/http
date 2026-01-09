<?php
/**
 * Signalforge HTTP Extension
 * Response.stub.php - IDE stub for Response class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * High-performance HTTP Response handler implementing PSR-7 ResponseInterface.
 *
 * Provides immutable response objects with zero-copy optimizations.
 *
 * @final
 * @implements \Psr\Http\Message\ResponseInterface
 */
final class Response implements \Psr\Http\Message\ResponseInterface
{
    /**
     * Private constructor - use Response::create() or factory methods instead.
     */
    private function __construct() {}

    /**
     * Create a new Response instance.
     *
     * @param int $status HTTP status code (default: 200)
     * @param array<string, string|string[]> $headers Response headers
     * @param string|\Psr\Http\Message\StreamInterface|null $body Response body
     * @return static
     * @throws \InvalidArgumentException for invalid status codes
     */
    public static function create(int $status = 200, array $headers = [], $body = null): static {}

    /**
     * Create a JSON response.
     *
     * @param mixed $data Data to encode as JSON
     * @param int $status HTTP status code (default: 200)
     * @return static
     * @throws \RuntimeException if JSON encoding fails
     */
    public static function json($data, int $status = 200): static {}

    /**
     * Create a text/plain response.
     *
     * @param string $text Text content
     * @param int $status HTTP status code (default: 200)
     * @return static
     */
    public static function text(string $text, int $status = 200): static {}

    /**
     * Create a text/html response.
     *
     * @param string $html HTML content
     * @param int $status HTTP status code (default: 200)
     * @return static
     */
    public static function html(string $html, int $status = 200): static {}

    /**
     * Create a redirect response.
     *
     * @param string $url Redirect URL
     * @param int $status HTTP status code (default: 302, must be 300-399)
     * @return static
     * @throws \InvalidArgumentException for invalid redirect status codes
     */
    public static function redirect(string $url, int $status = 302): static {}

    /* ============================================================================
     * PSR-7 ResponseInterface
     * ============================================================================ */

    /**
     * Gets the response status code.
     *
     * @return int Status code
     */
    public function getStatusCode(): int {}

    /**
     * Return an instance with the specified status code and, optionally, reason phrase.
     *
     * @param int $code The 3-digit integer result code to set
     * @param string $reasonPhrase The reason phrase to use with the provided status code
     * @return static
     * @throws \InvalidArgumentException for invalid status codes
     */
    public function withStatus(int $code, string $reasonPhrase = ''): static {}

    /**
     * Gets the response reason phrase associated with the status code.
     *
     * @return string Reason phrase
     */
    public function getReasonPhrase(): string {}

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
     * OUTPUT METHODS
     * ============================================================================ */

    /**
     * Send the response (headers and body).
     *
     * @return void
     */
    public function send(): void {}

    /**
     * Send only the response headers.
     *
     * @return void
     */
    public function sendHeaders(): void {}

    /**
     * Send only the response body.
     *
     * @return void
     */
    public function sendBody(): void {}

    /**
     * Serialize response to HTTP message string format.
     *
     * @return string HTTP message string
     */
    public function __toString(): string {}
}

