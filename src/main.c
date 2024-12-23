#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define EPSILON_SYMBOL '$'

typedef enum ExprNodeType ExprNodeType;
typedef struct ExprNode ExprNode;
typedef enum StateType StateType;
typedef enum TransitionType TransitionType;
typedef struct State State;
typedef struct Transition Transition;
typedef struct NFA NFA;

/**
 *  ******************************************************
 *                      NFA BUILDER
 *  ******************************************************
 */
 
enum ExprNodeType {
    SIMPLE,
    UNION,
    CONCAT,
    STAR
};

struct ExprNode {
    char symbol;
    ExprNodeType type;
    ExprNode *child[2];
};

enum StateType { 
    EPSILON,
    IMPORTANT,
    NONE
};

struct State {
    char symbol;
    StateType type;
    State *next[2];
};

struct NFA {
    State *start_state;
    State *accept_state;
};  

FILE *viz;

NFA simple_expr(ExprNode *node);
NFA union_expr(ExprNode *node);
NFA concat_expr(ExprNode *node);
NFA star_expr(ExprNode *node);
NFA make_nfa(ExprNode *node);

NFA simple_expr(ExprNode *node) {
    State *q = (State *) malloc(sizeof(State));
    State *f = (State *) malloc(sizeof(State));

    q->symbol = node->symbol;
    q->type = (node->symbol == EPSILON_SYMBOL) ? EPSILON : IMPORTANT;
    q->next[0] = f;
    q->next[1] = NULL;

    f->type = NONE;
    f->next[0] = NULL;
    f->next[1] = NULL;

    NFA nfa;
    nfa.start_state = q;
    nfa.accept_state = f;

    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", nfa.start_state, nfa.accept_state, node->symbol);

    return nfa;
}

NFA union_expr(ExprNode *node) {
    State *q = (State *) malloc(sizeof(State));
    State *f = (State *) malloc(sizeof(State));

    q->symbol = EPSILON_SYMBOL;
    q->type = EPSILON;

    f->symbol = EPSILON_SYMBOL;
    f->type = NONE;
    f->next[0] = NULL;
    f->next[1] = NULL;

    NFA n0 = make_nfa(node->child[0]);
    NFA n1 = make_nfa(node->child[1]);

    q->next[0] = n0.start_state;
    q->next[1] = n1.start_state;

    n0.accept_state->type = EPSILON;
    n0.accept_state->symbol = EPSILON_SYMBOL;
    n0.accept_state->next[0] = f;
    n0.accept_state->next[1] = NULL;

    n1.accept_state->type = EPSILON;
    n1.accept_state->symbol = EPSILON_SYMBOL;
    n1.accept_state->next[0] = f;
    n1.accept_state->next[1] = NULL;

    NFA nfa;
    nfa.start_state = q;
    nfa.accept_state = f;

    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", nfa.start_state, n0.start_state, EPSILON_SYMBOL);
    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", nfa.start_state, n1.start_state, EPSILON_SYMBOL);
    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", n0.accept_state, nfa.accept_state, EPSILON_SYMBOL);
    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", n1.accept_state, nfa.accept_state, EPSILON_SYMBOL);
    
    return nfa;
}

NFA concat_expr(ExprNode *node) {
    NFA n0 = make_nfa(node->child[0]);
    NFA n1 = make_nfa(node->child[1]);
    n0.accept_state->type = EPSILON;
    n0.accept_state->symbol = EPSILON_SYMBOL;
    n0.accept_state->next[0] = n1.start_state;

    NFA nfa;
    nfa.start_state = n0.start_state;
    nfa.accept_state = n1.accept_state;

    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", n0.accept_state, n1.start_state, EPSILON_SYMBOL);

    return nfa;
}

NFA star_expr(ExprNode *node) {
    State *q = (State *) malloc(sizeof(State));
    State *f = (State *) malloc(sizeof(State));

    NFA n0 = make_nfa(node->child[0]);

    q->type = EPSILON;
    q->symbol = EPSILON_SYMBOL;
    q->next[0] = n0.start_state;
    q->next[1] = f;

    f->type = NONE;

    n0.accept_state->type = EPSILON;
    n0.accept_state->symbol = EPSILON_SYMBOL;
    n0.accept_state->next[0] = n0.start_state;
    n0.accept_state->next[1] = f;

    NFA nfa;
    nfa.start_state = q;
    nfa.accept_state = f;

    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", nfa.start_state, n0.start_state, EPSILON_SYMBOL);
    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", n0.accept_state, n0.start_state, EPSILON_SYMBOL);
    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", n0.accept_state, nfa.accept_state, EPSILON_SYMBOL);
    fprintf(viz, "s%p->s%p [label=\"%c\"]\n\t", nfa.start_state, nfa.accept_state, EPSILON_SYMBOL);

    return nfa;
}

NFA make_nfa(ExprNode *node) {
    switch (node->type) {
        case UNION:
            return union_expr(node);
        case CONCAT:
            return concat_expr(node);
        case STAR:
            return star_expr(node);
        default: // simple
            return simple_expr(node);
    }
}

/**
 *  ******************************************************
 *                      REGEX PARSER
 *  ******************************************************
 */
typedef struct TokenStream TokenStream;

struct TokenStream {
    char *str;
    int index;
};

char look(TokenStream *lex, int forward) {
    return lex->str[lex->index + forward - 1];
}

void consume(TokenStream *lex) {
    lex->index++;
}

void error(char msg[]) {
    printf("%s\n", msg);
    exit(0);
}

bool is_id(char ch) {
    return (ch != '|' && ch != '*' && ch != '(' && ch != ')');
}

ExprNode *parse_expr(TokenStream *lex);
ExprNode *parse_operand(TokenStream *lex);
ExprNode *parse_unary_operator(TokenStream *lex);
ExprNode *parse_binary_operator(TokenStream *lex);

ExprNode *parse_expr(TokenStream *lex) {
    ExprNode *node_lhs = parse_operand(lex);
    
    if (look(lex, 1) == '\0' || look(lex, 1) == ')')
        return node_lhs;
    
    ExprNode *node_op = NULL;
    if (look(lex, 1) == '*') {
        node_op = parse_unary_operator(lex);
        node_op->child[0] = node_lhs;
        return node_op;
    }

    if (look(lex, 1) == '|' || is_id(look(lex, 1)) || look(lex, 1) == '(') {
        node_op = parse_binary_operator(lex);
    }
    else {
        error("Syntax error: Missing operator.");
    }

    ExprNode *node_rhs = parse_operand(lex);
    node_op->child[0] = node_lhs;
    node_op->child[1] = node_rhs;
    return node_op;
}

ExprNode *parse_operand(TokenStream *lex) {
    char next = look(lex, 1);
    if (!is_id(next) && next != '(') {
        error("Syntax error: Missing operand.");
        return NULL;
    }

    consume(lex);
    ExprNode *node = (ExprNode *) malloc(sizeof(ExprNode));
    node->child[0] = NULL;
    node->child[1] = NULL;

    if (is_id(next)) {
        node->type = SIMPLE;
        node->symbol = next;
    }
    else {
        node = parse_expr(lex);
        if (look(lex, 1) != ')')
            error("Syntax error: Missing closing parenthesis.");
        consume(lex);
    }
    return node;
}

ExprNode *parse_unary_operator(TokenStream *lex) {
    ExprNode *node = (ExprNode *) malloc(sizeof(ExprNode));
    node->type = STAR;
    node->symbol = '*';
    node->child[0] = NULL;
    node->child[1] = NULL;
    consume(lex);
    return node;
}

ExprNode *parse_binary_operator(TokenStream *lex) {
    ExprNode *node = (ExprNode *) malloc(sizeof(ExprNode));
    node->child[0] = NULL;
    node->child[1] = NULL;
    if (look(lex, 1) == '|') {
        node->type = UNION;
        node->symbol = '|'; 
        consume(lex);
    }
    else if (is_id(look(lex, 1)) || look(lex, 1) == '(') {
        node->type = CONCAT;
        node->symbol = '.'; 
    }
    return node;
}

void print_syntax_tree(ExprNode *root) {
    if (!root) return;

    printf("%c\n", root->symbol);
    print_syntax_tree(root->child[0]);
    print_syntax_tree(root->child[1]);
}

int main() {

    TokenStream lex;
    lex.index = 0;
    lex.str = "(((((a|b)*)|(c*))|(v|((m*)*)))*)p";
    printf("INPUT: %s\n\n", lex.str);

    ExprNode *root = parse_expr(&lex);
    printf("PARSE TREE:\n");
    print_syntax_tree(root);
    printf("\n");

    viz = fopen("NFA.dot", "w");
    fprintf(viz, "\0");
    fclose(viz);
    viz = fopen("NFA.dot", "a");
    fprintf(viz, "digraph {\n\tnode[label=\"\", shape=\"circle\"]\n\trankdir=\"LR\"\n\t");

    NFA nfa = make_nfa(root);

    fprintf(viz, "s%p [label=\"q\"]\n\t", nfa.start_state);
    fprintf(viz, "s%p [label=\"f\", shape=\"doublecircle\"]\n\t", nfa.accept_state);
    fprintf(viz, "}");

    fclose(viz);

    return 0;
}
