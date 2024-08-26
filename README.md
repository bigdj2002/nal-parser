# NAL-Parser Library

## Overview

The NAL-Parser library is currently under development and is based on reference software codes for standard codecs: H.264/AVC, H.265/HEVC, and H.266/VVC. The primary purpose of this library is to parse Non-VCL (Video Coding Layer) layers. Note that parsing of VCL layers is not within the scope of this library.

**Note: This library is still in development and not ready for production use. Contributions and feedback are welcome.**

## Features

- **Standard Codec Support**: 
  - H.264/AVC
  - H.265/HEVC
  - H.266/VVC
- **Non-VCL Layer Parsing**: Efficiently parses Non-VCL layers from the specified codecs.
- **Encoded Bitstream Input**: Only accepts encoded bitstreams as input.

### Requirements

- C++11 or later
