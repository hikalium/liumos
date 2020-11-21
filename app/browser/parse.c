// This is a part of "13.2.6 Tree construction" in the HTML spec.
// https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

#include "parse.h"

#include "../liumlib/liumlib.h"
#include "tokenize.h"

void PushStack(Node *node) {
  stack_of_open_elements[stack_index] = node;
  stack_index++;
}

Node *PopStack() {
  if (stack_index > 0)
    stack_index--;
  return stack_of_open_elements[stack_index];
}

Node *PopStackUntil(ElementType type) {
  while (stack_index > 0) {
    stack_index--;
    if (type == stack_of_open_elements[stack_index]->element_type)
      return stack_of_open_elements[stack_index];
  }
  return stack_of_open_elements[stack_index];
}

Node *CurrentNode() {
  return stack_of_open_elements[stack_index - 1];
}

// https://html.spec.whatwg.org/multipage/parsing.html#insert-a-foreign-element
void InsertElement(Node *element) {
  // "1. Let the adjusted insertion location be the appropriate place for inserting a node."
  // https://html.spec.whatwg.org/multipage/parsing.html#appropriate-place-for-inserting-a-node
  Node *target = CurrentNode();

  // "2. Let element be the result of creating an element for the token in the given
  // namespace, with the intended parent being the element in which the adjusted
  // insertion location finds itself."
  element->parent = target;

  // "3. If it is possible to insert element at the adjusted insertion location, then:
  //  3.2. Insert element at the adjusted insertion location."
  //
  // https://html.spec.whatwg.org/multipage/parsing.html#appropriate-place-for-inserting-a-node
  // "2. Let adjusted insertion location be inside target, after its last child (if any)."
  if (target->last_child == NULL) {
    target->first_child = element;
    target->last_child = element;
  } else {
    Node *ex_last_child = target->last_child;
    target->last_child = element;

    // Connect siblings.
    ex_last_child->next_sibling = element;
    element->previous_sibling = ex_last_child;
  }

  // "4. Push element onto the stack of open elements so that it is the new current node."
  PushStack(element);
}

// https://html.spec.whatwg.org/multipage/parsing.html#insert-a-character
void InsertCharFromToken(Token *token, bool inserting_char) {
  // "1. Let data be the characters passed to the algorithm, or, if no characters were
  // explicitly specified, the character of the character token being processed."

  // "2. Let the adjusted insertion location be the appropriate place for inserting a node."
  Node *target = CurrentNode();

  // "3. If the adjusted insertion location is in a Document node, then return."
  if (target->element_type == DOCUMENT)
    return;

  // "4. If there is a Text node immediately before the adjusted insertion location,
  // then append data to that Text node's data."
  if (target->element_type == TEXT) {
    strcat(CurrentNode()->data, &token->data);
  } else {
    Node *node = CreateText(token);
    InsertElement(node);
  }
}

// https://html.spec.whatwg.org/multipage/dom.html#document
Node *CreateDocument() {
  Node *node = (Node *) malloc(sizeof(Node));
  node->element_type = DOCUMENT;
  node->tag_name = NULL;
  node->attributes = NULL;
  node->data = NULL;
  node->first_child = NULL;
  node->last_child = NULL;
  node->previous_sibling = NULL;
  node->next_sibling = NULL;
  return node;
}

Node *CreateElement(ElementType element_type, char *tag_name) {
  Node *node = (Node *) malloc(sizeof(Node));
  node->element_type = element_type;
  if (tag_name)
    node->tag_name = tag_name;
  node->attributes = NULL;
  node->data = NULL;
  node->first_child = NULL;
  node->last_child = NULL;
  node->previous_sibling = NULL;
  node->next_sibling = NULL;
  return node;
}

// https://html.spec.whatwg.org/multipage/parsing.html#create-an-element-for-the-token
Node *CreateElementFromToken(ElementType element_type, Token *token) {
  Node *node = (Node *) malloc(sizeof(Node));
  node->element_type = element_type;
  // "2. Let local name be the tag name of the token."
  if (token->tag_name)
    node->tag_name = token->tag_name;
  if (token->attributes)
    node->attributes = token->attributes;
  node->data = NULL;
  node->first_child = NULL;
  node->last_child = NULL;
  node->previous_sibling = NULL;
  node->next_sibling = NULL;
  return node;
}

Node *CreateText(Token *token) {
  Node *node = (Node *) malloc(sizeof(Node));
  node->element_type = TEXT;
  node->attributes = NULL;
  char *data = (char *) malloc(100);
  data[0] = token->data;
  data[1] = '\0';
  node->data = data;
  node->first_child = NULL;
  node->last_child = NULL;
  node->previous_sibling = NULL;
  node->next_sibling = NULL;
  return node;
}

void ConstructTree() {
  Mode mode = INITIAL;

  Node *document = CreateDocument();
  root_node = document;

  stack_index = 0;
  PushStack(document);

  Token *token = first_token;

  bool inserting_char = false;

  while (token) {
    switch (mode) {
      case INITIAL:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
        mode = BEFORE_HTML;
        break;
      case BEFORE_HTML:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
        if (token->type == DOCTYPE) {
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == CHAR &&
            (token->data == 0x09 /* Tab */ ||
            token->data == 0x0a /* LF */ ||
            token->data == 0x0c /* FF */ ||
            token->data == 0x0d /* CR */ ||
            token->data == 0x20 /* Space */)) {
          // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
          // Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "html") == 0) {
          // A start tag whose tag name is "html"
          // Create an element for the token in the HTML namespace, with the Document as the intended parent. Append it to the Document object. Put this element in the stack of open elements.
          Node *element = CreateElementFromToken(HTML, token);
          InsertElement(element);
          mode = BEFORE_HEAD;
          token = token->next;
          break;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "head") != 0 ||
             strcmp(token->tag_name, "body") != 0 ||
             strcmp(token->tag_name, "html") != 0 ||
             strcmp(token->tag_name, "br") != 0)) {
          // Any other end tag
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        // Anything else
        {
          Node *element = CreateElement(HTML, "html");
          InsertElement(element);
        }
        mode = BEFORE_HEAD;
        // Reprocess the token.
        break;
      case BEFORE_HEAD:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
        if (token->type == CHAR &&
            (token->data == 0x09 /* Tab */ ||
            token->data == 0x0a /* LF */ ||
            token->data == 0x0c /* FF */ ||
            token->data == 0x0d /* CR */ ||
            token->data == 0x20 /* Space */)) {
          // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
          // Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == DOCTYPE) {
          // A DOCTYPE token
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "head") == 0) {
          // A start tag whose tag name is "head"
          Node *element = CreateElementFromToken(HEAD, token);
          InsertElement(element);
          mode = IN_HEAD;
          token = token->next;
          break;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "head") != 0 ||
             strcmp(token->tag_name, "body") != 0 ||
             strcmp(token->tag_name, "html") != 0 ||
             strcmp(token->tag_name, "br") != 0)) {
          // Any other end tag
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        // Anything else
        {
          Node *element = CreateElement(HEAD, "head");
          InsertElement(element);
        }
        mode = IN_HEAD;
        break;
      case IN_HEAD:
        // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
        if (token->type == DOCTYPE) {
          // A DOCTYPE token
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "html") == 0) {
          // A start tag whose tag name is "html"
          // Process the token using the rules for the "in body" insertion mode.
          mode = IN_BODY;
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "head") == 0) {
          // A start tag whose tag name is "head"
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == END_TAG && strcmp(token->tag_name, "head") == 0) {
          // An end tag whose tag name is "head"
          // Pop the current node (which will be the head element) off the stack of open elements.
          PopStackUntil(HEAD);
          token = token->next;
          mode = AFTER_HEAD;
          break;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "body") != 0 ||
            strcmp(token->tag_name, "html") != 0 ||
            strcmp(token->tag_name, "br") != 0)) {
          // Any other end tag
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        // Anything else
        PopStackUntil(HEAD);
        mode = AFTER_HEAD;
        // Reprocess the token.
        break;
      case AFTER_HEAD:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-after-head-insertion-mode
        if (token->type == DOCTYPE) {
          // A DOCTYPE token
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "html") == 0) {
          // A start tag whose tag name is "html"
          // Process the token using the rules for the "in body" insertion mode.
          mode = IN_BODY;
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "body") == 0) {
          // A start tag whose tag name is "body"
          Node *element = CreateElementFromToken(BODY, token);
          InsertElement(element);
          mode = IN_BODY;
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "head") == 0) {
          // A start tag whose tag name is "head"
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "body") != 0 ||
            strcmp(token->tag_name, "html") != 0 ||
            strcmp(token->tag_name, "br") != 0)) {
          // Any other end tag
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        // Anything else
        {
          Node *element = CreateElement(BODY, "body");
          InsertElement(element);
        }
        mode = IN_BODY;
        // Reprocess the token.
        break;
      case IN_BODY:
        // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
        if (token->type == CHAR) {
          InsertCharFromToken(token, inserting_char);
          token = token->next;
          inserting_char = true;
          break;
        }
        inserting_char = false;
        if (token->type == DOCTYPE) {
          // A DOCTYPE token
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == START_TAG &&
            (strcmp(token->tag_name, "body") == 0 ||
            strcmp(token->tag_name, "html") == 0)) {
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == EOF) {
          // An end-of-file token
          // Stop parsing.
          return;
        }
        if (token->type == END_TAG && strcmp(token->tag_name, "body") == 0) {
          // An end tag whose tag name is "body"
          PopStackUntil(BODY);
          mode = AFTER_BODY;
          token = token->next;
          break;
        }
        if (token->type == END_TAG && strcmp(token->tag_name, "html") == 0) {
          // An end tag whose tag name is "html"
          PopStackUntil(HTML);
          mode = AFTER_BODY;
          // Reprocess the token.
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "ul") == 0) {
          Node *element = CreateElementFromToken(UL, token);
          InsertElement(element);
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "p") == 0) {
          Node *element = CreateElementFromToken(PARAGRAPH, token);
          InsertElement(element);
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "div") == 0) {
          Node *element = CreateElementFromToken(DIV, token);
          InsertElement(element);
          token = token->next;
          break;
        }
        if (token->type == START_TAG &&
            (strcmp(token->tag_name, "h1") == 0 ||
            strcmp(token->tag_name, "h2") == 0 ||
            strcmp(token->tag_name, "h3") == 0 ||
            strcmp(token->tag_name, "h4") == 0 ||
            strcmp(token->tag_name, "h5") == 0 ||
            strcmp(token->tag_name, "h6") == 0)) {
          // A start tag whose tag name is one of: "h1", "h2", "h3", "h4", "h5", "h6"
          Node *element = CreateElementFromToken(HEADING, token);
          InsertElement(element);
          token = token->next;
          break;
        }
        if (token->type == START_TAG && strcmp(token->tag_name, "li") == 0) {
          // A start tag whose tag name is "li"
          Node *element = CreateElementFromToken(LI, token);
          InsertElement(element);
          token = token->next;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "div") == 0)) {
          PopStackUntil(DIV);
          token = token->next;
          break;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "ul") == 0)) {
          PopStackUntil(UL);
          token = token->next;
          break;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "p") == 0)) {
          PopStackUntil(PARAGRAPH);
          token = token->next;
          break;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "li") == 0)) {
          PopStackUntil(LI);
          token = token->next;
          break;
        }
        if (token->type == END_TAG &&
            (strcmp(token->tag_name, "h1") == 0 ||
            strcmp(token->tag_name, "h2") == 0 ||
            strcmp(token->tag_name, "h3") == 0 ||
            strcmp(token->tag_name, "h4") == 0 ||
            strcmp(token->tag_name, "h5") == 0 ||
            strcmp(token->tag_name, "h6") == 0)) {
         // An end tag whose tag name is one of: "h1", "h2", "h3", "h4", "h5", "h6"
          PopStackUntil(HEADING);
          token = token->next;
          break;
        }
        if (token->type == END_TAG) {
          // Any other end tag
          PopStack();
          token = token->next;
          break;
        }
        break;
      case AFTER_BODY:
        // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-afterbody
        if (token->type == DOCTYPE) {
          // A DOCTYPE token
          // Parse error. Ignore the token.
          token = token->next;
          break;
        }
        if (token->type == END_TAG && strcmp(token->tag_name, "html") == 0) {
          // An end tag whose tag name is "html"
          token = token->next;
          mode = AFTER_AFTER_BODY;
          break;
        }
        if (token->type == EOF) {
          // An end-of-file token
          // Stop parsing.
          return;
        }
        // Anything else
        // Parse error. Switch the insertion mode to "in body" and reprocess the token.
        mode = IN_BODY;
        break;
      case AFTER_AFTER_BODY:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-after-after-body-insertion-mode
        if (token->type == EOF) {
          // An end-of-file token
          // Stop parsing.
          return;
        }
        // Anything else
        // Parse error. Switch the insertion mode to "in body" and reprocess the token.
        mode = IN_BODY;
        break;
    }
  }
}

// for debug.
void PrintNode(Node *node, int depth) {
  if (node == NULL)
    return;

  PrintNum(depth);
  Print(":");
  for (int i=0; i<depth; i++) {
    Print(" ");
  }
  Print(":");
  switch (node->element_type) {
    case DOCUMENT:
      Println("DOCUMENT");
      break;
    case HTML:
      Println("HTML");
      break;
    case HEAD:
      Println("HEAD");
      break;
    case BODY:
      Println("BODY");
      break;
    case TEXT:
      Print("text: ");
      Println(node->data);
      break;
    default:
      Print("node: ");
      Println(node->tag_name);
      break;
  }
}

// for debug.
void DfsWithDepth(Node *node, int depth) {
  PrintNode(node, depth);

  for (Node *cur = node->first_child; cur; cur = cur->next_sibling) {
    DfsWithDepth(cur, depth + 1);
  }
}

// for debug.
void PrintNodes() {
  Println("--------------");
  DfsWithDepth(root_node, 0);
  Println("--------------");
}
