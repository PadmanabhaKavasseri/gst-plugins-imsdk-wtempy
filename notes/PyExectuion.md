**What is Bytecode & How Python Functions Become Bytecode:**

## **What is Bytecode:**
• **Intermediate representation:** Platform-independent instructions between source code and machine code
• **Virtual machine opcodes:** Simple instructions like LOAD, STORE, ADD, CALL
• **Stack-based:** Operations work on a virtual stack (push/pop values)
• **Portable:** Same bytecode runs on any platform with Python interpreter
• **Cached:** Stored in .pyc files to avoid recompilation

## **Python → Bytecode Conversion Process:**

### **1. Lexical Analysis (Tokenization):**
```python
def add(a, b):
    return a + b
```
**Becomes tokens:** `DEF`, `NAME(add)`, `LPAREN`, `NAME(a)`, `COMMA`, `NAME(b)`, `RPAREN`, `COLON`, `RETURN`, `NAME(a)`, `PLUS`, `NAME(b)`

### **2. Parsing (AST Generation):**
**Creates Abstract Syntax Tree:**
```
FunctionDef(name='add')
├── arguments: [arg(a), arg(b)]
└── body: Return
    └── BinOp
        ├── left: Name(a)
        ├── op: Add()
        └── right: Name(b)
```

### **3. Code Generation (AST → Bytecode):**
**Converts AST to bytecode instructions:**
```
  0 LOAD_FAST     0 (a)      # Push 'a' onto stack
  2 LOAD_FAST     1 (b)      # Push 'b' onto stack  
  4 BINARY_ADD               # Pop two values, add, push result
  6 RETURN_VALUE             # Return top of stack
```

## **Bytecode Instructions Explained:**

### **Common Opcodes:**
• **LOAD_FAST(n):** Push local variable n onto stack
• **LOAD_GLOBAL(name):** Push global variable onto stack
• **STORE_FAST(n):** Pop stack, store in local variable n
• **BINARY_ADD:** Pop two values, add them, push result
• **CALL_FUNCTION(argc):** Call function with argc arguments
• **RETURN_VALUE:** Return top of stack as function result

### **Stack Operations Example:**
```python
x = a + b * c
```
**Bytecode:**
```
LOAD_FAST     0 (a)        # Stack: [a]
LOAD_FAST     1 (b)        # Stack: [a, b]
LOAD_FAST     2 (c)        # Stack: [a, b, c]
BINARY_MULTIPLY            # Stack: [a, (b*c)]
BINARY_ADD                 # Stack: [(a + b*c)]
STORE_FAST    3 (x)        # Stack: [], x = result
```

## **When Conversion Happens:**

### **Import Time:**
• **First import:** `.py` file compiled to bytecode, cached as `.pyc`
• **Subsequent imports:** Load cached `.pyc` if newer than `.py`
• **Module loading:** `PyImport_ImportModule()` triggers compilation
• **Function definition:** Creates code object with bytecode

### **In Your Plugin:**
```c
// When you call this:
PyObject* module = PyImport_ImportModule("opencv_processor");

// Python internally does:
// 1. Read opencv_processor.py
// 2. Parse into AST  
// 3. Compile AST to bytecode
// 4. Create module object with bytecode
// 5. Execute module-level code (imports, function definitions)
```

## **Code Objects:**

### **What Gets Stored:**
```c
typedef struct {
    PyObject_HEAD
    unsigned char *co_code;      // Bytecode instructions
    PyObject *co_names;          // Global variable names
    PyObject *co_varnames;       // Local variable names  
    PyObject *co_consts;         // Constants (numbers, strings)
    int co_argcount;             // Number of arguments
    // ... more metadata
} PyCodeObject;
```

### **Your Function Object Contains:**
```c
PyFunctionObject {
    PyCodeObject *func_code;     // ← The compiled bytecode lives here
    PyObject *func_globals;      // Global namespace
    PyObject *func_name;         // "mobilenet_preprocess"
}
```

## **Execution Process:**

### **When You Call Python Function:**
```c
PyObject_CallObject(function, args)
```

**Python VM does:**
1. **Extract bytecode** from `function->func_code->co_code`
2. **Set up execution frame** with local variables
3. **Execute bytecode loop:**
   ```c
   while (1) {
       opcode = *bytecode_ptr++;
       switch (opcode) {
           case LOAD_FAST: /* push local var */ break;
           case BINARY_ADD: /* pop, add, push */ break;
           case RETURN_VALUE: return stack_top;
       }
   }
   ```

**Key Point:** Bytecode is the "assembly language" of the Python virtual machine - it's what actually gets executed when your Python function runs!