# 3. Implement response as variant

Date: 2021-02-12

## Status

Accepted

## Context

The number of responses is highly stable.
If we ever add another response, the compiler should help us to identify all places where we have to support it.

## Decision

We change the implementation of a response from an enumeration class to a variant based type.

## Consequences

A conversion to/from text is achievable without help from the json library.
The use of response variables might become less readable because of the incredibly general visit syntax.
