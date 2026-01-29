## Pipeline Startup Sequence

### 1. __Pipeline Construction__

```bash
Element created → gst_tempy_init() called
```

### 2. __State Change: NULL → READY__
```bash
→ start()                    // Initialize resources, load Python
```

### 3. __Caps Negotiation Phase__

```bash
→ transform_caps()           // Convert input caps to output caps
→ fixate_caps()             // Choose specific caps from ranges
→ set_caps()                // Finalize negotiated caps
→ decide_allocation()        // Set up buffer pools
```

### 4. __Processing Phase (Repeated for each buffer)__

```bash
→ query() [optional]         // Handle queries from downstream
→ prepare_output_buffer()    // Get output buffer from pool
→ transform()               // Process the actual data
```

### 5. __Pipeline Shutdown__

```bash
→ stop()                    // Clean up resources, cleanup Python
```

## Functions That May Be Called Multiple Times

### During Caps Renegotiation:

- `transform_caps()` - If upstream changes format
- `fixate_caps()` - If caps need to be renegotiated
- `set_caps()` - If new caps are negotiated
- `decide_allocation()` - If buffer requirements change

### During Processing:

- `query()` - Anytime downstream asks questions
- `prepare_output_buffer()` - Once per frame
- `transform()` - Once per frame




