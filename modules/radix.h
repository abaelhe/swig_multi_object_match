#ifndef RADIXTREE
#define RADIXTREE

#define RADIXTREE_KEYSIZE 128

//核心理念:  把 key 看成bit 序列, `bit stream` 或 bitmap, 而非通常意义上的 `字符串`;
//一个字节: [左边为Most Significant Bit]  M:1,1,1,1, 1,1,1,1:L  [最右为 Least Significant Bit]

// node->pos: key比特数index:  (strlen(key) + 1)*8 -1
// node->color: 节点类型:      {1:leaf, 0:inner, 2:proxy_node },  proxy_node: proxy_node->value 是后继 node 节点的指针;

// color:

typedef struct rxt_node {
    int color; //0: left != NULL && right != NULL, 1: for leaf, 2: ->left==NULL && right==NULL,
    char *key;
    void *value;
    int pos; // bit index of the key to compare at (critical position)
    char keycache[RADIXTREE_KEYSIZE];
    int level; // tree level; for debug only
    int parent_id; //for debug only
    struct rxt_node *parent;
    struct rxt_node *left;
    struct rxt_node *right;
}rxt_node;

int rxt_put(char*, void *, rxt_node*);
void* rxt_get(char*, rxt_node*);
void* rxt_delete(char*, rxt_node*);
void rxt_free(rxt_node *);
rxt_node *rxt_init();

#endif // RADIXTREE
