# PRIMITIVE MPEG-TS DEMUXER [![Build Status](https://travis-ci.org/t0k3n1z3r/ts-proc.svg?branch=master)](https://travis-ci.org/t0k3n1z3r/ts-proc)

## Description
This project represents simple TS demuxer to extract video and audio bitstreams to separate files. At the moment it supports only SPTS (Single Program Transport Stream). Tested only on couple of files with H.264 and aac inside.

## Build & Run
Just execute make. Binary called ts-proc and it takes 3 arguments on input such as <input_file>,
<out_video_file> and <out_audio_file>. Example: ts-proc data/elephants.ts video.264 audio.aac

## Known limitations
- No support for MPTS
- Limited support of broken input (validates only sync byte)
- Doesn't rewind at the beginning when PAT and PMT found
- Supports only file as an input
- No extensive validation of TS structure (assumption that stream is OK)
