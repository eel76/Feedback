# 4. Do file IO without memory mapping

Date: 2021-02-17

## Status

Accepted

## Context

We evaluated mio (https://github.com/mandreyel/mio) to improve overall file IO performance.
The measured performance was not better but comparable or even worse.
We ran into reproducible crashes for certain files.

## Decision

We do file IO without memory mapping.

## Consequences

File IO operations do not require an additional third party library.
This decision reduces maintainment costs and improves cross platform availability.
