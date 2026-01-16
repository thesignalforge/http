<?php
/**
 * Signalforge HTTP Extension
 * Uri.stub.php - IDE stub for Uri class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * High-performance URI handler implementing PSR-7 UriInterface.
 *
 * Provides RFC 3986 compliant URI parsing with immutable modification methods.
 *
 * @final
 * @implements \Psr\Http\Message\UriInterface
 * @implements \Stringable
 */
final class Uri implements \Psr\Http\Message\UriInterface, \Stringable
{
    /**
     * Private constructor - use Uri::fromString() factory method instead.
     */
    private function __construct() {}

    /**
     * Create a Uri instance from a URI string.
     *
     * @param string $uri URI string to parse
     * @return static
     * @throws \InvalidArgumentException If the URI cannot be parsed
     */
    public static function fromString(string $uri): static {}

    /* ============================================================================
     * PSR-7 UriInterface - Getters
     * ============================================================================ */

    /**
     * Retrieve the scheme component of the URI.
     *
     * @return string The URI scheme (e.g., "http", "https"), or empty string if not present
     */
    public function getScheme(): string {}

    /**
     * Retrieve the authority component of the URI.
     *
     * Format: [user-info@]host[:port]
     *
     * @return string The URI authority, or empty string if not present
     */
    public function getAuthority(): string {}

    /**
     * Retrieve the user information component of the URI.
     *
     * Format: username[:password]
     *
     * @return string The URI user information, or empty string if not present
     */
    public function getUserInfo(): string {}

    /**
     * Retrieve the host component of the URI.
     *
     * @return string The URI host (lowercase), or empty string if not present
     */
    public function getHost(): string {}

    /**
     * Retrieve the port component of the URI.
     *
     * @return int|null The URI port, or null if not present or is standard port for scheme
     */
    public function getPort(): ?int {}

    /**
     * Retrieve the path component of the URI.
     *
     * @return string The URI path
     */
    public function getPath(): string {}

    /**
     * Retrieve the query string of the URI.
     *
     * @return string The URI query string (without leading "?"), or empty string if not present
     */
    public function getQuery(): string {}

    /**
     * Retrieve the fragment component of the URI.
     *
     * @return string The URI fragment (without leading "#"), or empty string if not present
     */
    public function getFragment(): string {}

    /* ============================================================================
     * PSR-7 UriInterface - Immutable Modifiers
     * ============================================================================ */

    /**
     * Return an instance with the specified scheme.
     *
     * @param string $scheme The scheme to use (will be lowercased)
     * @return static A new instance with the specified scheme
     */
    public function withScheme(string $scheme): static {}

    /**
     * Return an instance with the specified user information.
     *
     * @param string $user The user name to use for authority
     * @param string|null $password The password associated with $user
     * @return static A new instance with the specified user information
     */
    public function withUserInfo(string $user, ?string $password = null): static {}

    /**
     * Return an instance with the specified host.
     *
     * @param string $host The hostname to use (will be lowercased)
     * @return static A new instance with the specified host
     */
    public function withHost(string $host): static {}

    /**
     * Return an instance with the specified port.
     *
     * @param int|null $port The port to use, or null to remove the port
     * @return static A new instance with the specified port
     * @throws \InvalidArgumentException For invalid port values (outside 0-65535)
     */
    public function withPort(?int $port): static {}

    /**
     * Return an instance with the specified path.
     *
     * @param string $path The path to use
     * @return static A new instance with the specified path
     * @throws \InvalidArgumentException If the path contains query string or fragment
     */
    public function withPath(string $path): static {}

    /**
     * Return an instance with the specified query string.
     *
     * @param string $query The query string to use (without leading "?")
     * @return static A new instance with the specified query string
     * @throws \InvalidArgumentException If the query string contains a fragment
     */
    public function withQuery(string $query): static {}

    /**
     * Return an instance with the specified URI fragment.
     *
     * @param string $fragment The fragment to use (without leading "#")
     * @return static A new instance with the specified fragment
     */
    public function withFragment(string $fragment): static {}

    /**
     * Return the string representation of the URI.
     *
     * @return string Complete URI string following RFC 3986
     */
    public function __toString(): string {}
}
