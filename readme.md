# Tempo

Tempo is an experimental functional programming language designed for music composition and sound synthesis.

Tempo projects can either be built into a .wav or .mp3 file (todo), or run interactively as an environment for experimentation and improvisation. Hot-reloading is supported, so changes to code are immedately reflected in what you hear.


## Programming model

Tempo models compositions, instruments and synthesizers as pure, stateless functions that return a sample amplitude for each audio frame.

We can represent a simple sin oscillator using a function such as:

```ml
sinOsc :: Float -> Float -> Float
sinOsc hz time =
  sin (hz * 2 * PI * time * SAMPLE_RATE)
;
```

and a composition that consists of just this sin oscillator playing forever at 220Hz:

```ml
main :: Float -> Float
main = sinOsc 220;
```

By providing the advanced capabilities for abstraction and composition of a modern functional langage, Tempo will allow this model to scale from individual synths, through the notes and bars, right up to a whole piece of music:

```ml
main :: Float -> Float
main =
  sequence [E, G, A, _] (bpm 120)
  >> sinOsc
```


## Status & Roadmap

Tempo is in a very early stage and is still lacking many features needed for it to become a viable environment for producing music.

☑︎ Static type system and type inference.

☑︎ Basic artitmetic and trig functions.

☑︎ Interactive mode with hot-reloading.

☐︎ Pattern-matching on time ranges.

☐ Module system, with support for referencing .wav and .mp3 files as samples.

☐︎ List, set & tuple data structures.

☐︎ User-defined record data types.

☐︎ Discriminated union types.

☐︎ Pattern-matching over union types.

☐ Export to .wav, .mp3, etc.

☐ MIDI events and user-input.

☐ Standard library.

☐ Library of basic synthesizers and effects.

☐ Library abstractions for sequencing & composition.

☐ Package manager.