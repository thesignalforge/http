<?php
/**
 * Signalforge HTTP Extension
 * Stream.stub.php - IDE stub for Stream class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * High-performance HTTP Stream handler implementing PSR-7 StreamInterface.
 *
 * Provides zero-copy string streams and efficient resource wrapping.
 *
 * @final
 * @implements \Psr\Http\Message\StreamInterface
 */
final class Stream implements \Psr\Http\Message\StreamInterface
{
    /**
     * Private constructor - use factory methods instead.
     */
    private function __construct() {}

    /**
     * Create a stream from a string (zero-copy reference).
     *
     * @param string $string String content
     * @return static
     */
    public static function fromString(string $string): static {}

    /**
     * Create a stream from a PHP stream resource.
     *
     * @param resource $resource PHP stream resource
     * @return static
     * @throws \InvalidArgumentException if resource is not a valid stream
     */
    public static function fromResource($resource): static {}

    /**
     * Create a stream from a file.
     *
     * @param string $path File path
     * @param string $mode File mode (default: 'r')
     * @return static
     * @throws \RuntimeException if file cannot be opened
     */
    public static function fromFile(string $path, string $mode = 'r'): static {}

    /* ============================================================================
     * PSR-7 StreamInterface
     * ============================================================================ */

    /**
     * Reads data from the stream.
     *
     * @param int $length Read up to $length bytes from the object and return them
     * @return string Returns the data read from the stream
     * @throws \RuntimeException if an error occurs
     */
    public function read(int $length): string {}

    /**
     * Returns the remaining contents in a string.
     *
     * @return string Returns the remaining contents
     * @throws \RuntimeException if unable to read or an error occurs while reading
     */
    public function getContents(): string {}

    /**
     * Returns whether or not the stream is readable.
     *
     * @return bool
     */
    public function isReadable(): bool {}

    /**
     * Write data to the stream.
     *
     * @param string $string The string that is to be written
     * @return int Returns the number of bytes written to the stream
     * @throws \RuntimeException on failure
     */
    public function write(string $string): int {}

    /**
     * Returns whether or not the stream is writable.
     *
     * @return bool
     */
    public function isWritable(): bool {}

    /**
     * Seek to a position in the stream.
     *
     * @param int $offset Stream offset
     * @param int $whence Specifies how the cursor position will be calculated (SEEK_SET, SEEK_CUR, SEEK_END)
     * @return void
     * @throws \RuntimeException on failure
     */
    public function seek(int $offset, int $whence = SEEK_SET): void {}

    /**
     * Returns the current position of the file read/write pointer.
     *
     * @return int Position of the file pointer
     * @throws \RuntimeException on error
     */
    public function tell(): int {}

    /**
     * Returns true if the stream is at the end of the stream.
     *
     * @return bool
     */
    public function eof(): bool {}

    /**
     * Seek to the beginning of the stream.
     *
     * @return void
     * @throws \RuntimeException on failure
     */
    public function rewind(): void {}

    /**
     * Returns whether or not the stream is seekable.
     *
     * @return bool
     */
    public function isSeekable(): bool {}

    /**
     * Get the size of the stream if known.
     *
     * @return int|null Returns the size in bytes if known, or null if unknown
     */
    public function getSize(): ?int {}

    /**
     * Get stream metadata as an associative array or retrieve a specific key.
     *
     * @param string|null $key Specific metadata to retrieve
     * @return array|mixed|null Returns an associative array if no key is provided, or a specific key value if a key is provided
     */
    public function getMetadata(?string $key = null) {}

    /**
     * Closes the stream and any underlying resources.
     *
     * @return void
     */
    public function close(): void {}

    /**
     * Separates any underlying resources from the stream.
     *
     * @return resource|null Underlying PHP stream, if any
     */
    public function detach() {}

    /**
     * Reads all data from the stream into a string.
     *
     * @return string
     */
    public function __toString(): string {}
}

