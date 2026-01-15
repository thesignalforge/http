<?php
/**
 * Signalforge HTTP Extension
 * StreamFactory.stub.php - IDE stub for StreamFactory class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * PSR-17 compliant stream factory.
 *
 * Creates StreamInterface instances from various sources.
 *
 * @final
 * @implements \Psr\Http\Message\StreamFactoryInterface
 */
final class StreamFactory implements \Psr\Http\Message\StreamFactoryInterface
{
    /**
     * Create a new stream from a string.
     *
     * @param string $content String content with which to populate the stream
     * @return \Psr\Http\Message\StreamInterface
     */
    public function createStream(string $content = ''): \Psr\Http\Message\StreamInterface {}

    /**
     * Create a stream from an existing file.
     *
     * @param string $filename Filename or stream URI to use as basis of stream
     * @param string $mode Mode with which to open the underlying filename/stream
     * @return \Psr\Http\Message\StreamInterface
     * @throws \RuntimeException If the file cannot be opened
     * @throws \InvalidArgumentException If the mode is invalid
     */
    public function createStreamFromFile(string $filename, string $mode = 'r'): \Psr\Http\Message\StreamInterface {}

    /**
     * Create a new stream from an existing resource.
     *
     * @param resource $resource PHP resource to use as basis of stream
     * @return \Psr\Http\Message\StreamInterface
     * @throws \InvalidArgumentException If the resource is not a valid stream resource
     */
    public function createStreamFromResource($resource): \Psr\Http\Message\StreamInterface {}
}
