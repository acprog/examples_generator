#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <vector>
#include <time.h>

static int examples_count=0;
static int items_count=0;

enum action_type{
  USE_ADD=1,
  USE_SUB=1<<1,
  USE_MUL=1<<2,
  USE_DEV=1<<3,
  USE_BRAKES=1<<4,
  USE_NUMBER=1<<5
};

static int use_actions=0;
static int max_brakes=0;
static int min_value=0;
static int max_value=0;

class action;
action *random_action(int count, int *brakes, int actions_mask);

//=====================================================================================================
// действие
//=====================================================================================================
class action {
public:
  virtual ~action(){}
  virtual int get_count()=0;
  virtual int result()=0;
  virtual void print()=0;
};

//=====================================================================================================
// значение
//=====================================================================================================
class number : public action {
public:
  //---------------------------------------------------------------------------------------------------
  number(int _value)
    :value (_value)
    ,is_x  (false)
  {
    numbers.push_back(this);
  }

  //---------------------------------------------------------------------------------------------------
  virtual ~number() {
    for (int i=0; i<numbers.size(); i++)
      if (numbers[i]==this) {
        numbers[i]=NULL;
        break;
      }
  }

  //---------------------------------------------------------------------------------------------------
  virtual int get_count() {
    return 1;
  }

  //---------------------------------------------------------------------------------------------------
  virtual int result() {
    return value;
  }

  //---------------------------------------------------------------------------------------------------
  virtual void print() {
    if (is_x)
      printf("X");
    else
      printf("%d", value);
  }

  //---------------------------------------------------------------------------------------------------
  static int set_x() {
    int x=0;
    while (numbers.size()) {
      int id=rand()%numbers.size();
      if (numbers[id]) {
        numbers[id]->is_x=true;
        x=numbers[id]->value;
        break;
      }
    }
    numbers.clear();
    return x;
  }

private:
  int value;
  bool is_x;

  static std::vector<number*> numbers;
};

std::vector<number*> number::numbers;


//=====================================================================================================
// скобки
//=====================================================================================================
class brakes_action : public action {
public:
  //---------------------------------------------------------------------------------------------------
  brakes_action(int count, int *brakes_count)
  {
    (*brakes_count)--;
    item=random_action(count, brakes_count, use_actions & ~(USE_BRAKES | USE_NUMBER));
  }

  //---------------------------------------------------------------------------------------------------
  virtual ~brakes_action() {
    delete item;
  }

  //---------------------------------------------------------------------------------------------------
  virtual int get_count() {
    return item->get_count();
  }

  //---------------------------------------------------------------------------------------------------
  virtual int result() {
    return item->result();
  }

  //---------------------------------------------------------------------------------------------------
  virtual void print() {
    printf(" ( ");
    item->print();
    printf(" ) ");
  }

private:
  action *item;
};


//=====================================================================================================
// сложение, список
//=====================================================================================================
class add_action : public action {
public:
  //---------------------------------------------------------------------------------------------------
  add_action(int count, int *brakes_count) {
    while (count>0) {
      action *act=random_action(rand()%count+1, brakes_count, use_actions & ~(USE_ADD | USE_SUB));
      if (act) {
        count-=act->get_count();
        subactions.push_back(act);
      }
    }
  }

  //---------------------------------------------------------------------------------------------------
  add_action(action *first, action *second) {
    subactions.push_back(first);
    subactions.push_back(second);
  }

  //---------------------------------------------------------------------------------------------------
  virtual ~add_action() {
    for (std::list<action*>::iterator i=subactions.begin(); i!=subactions.end(); ++i)
      delete *i;
  }

  //---------------------------------------------------------------------------------------------------
  int get_count() {
    int count=0;
    for (std::list<action*>::iterator i=subactions.begin(); i!=subactions.end(); ++i)
      count+=(*i)->get_count();
    return count;
  }

  //---------------------------------------------------------------------------------------------------
  virtual int result() {
    int value=0;
    for (std::list<action*>::iterator i=subactions.begin(); i!=subactions.end(); ++i)
      value+=(*i)->result();
    return value;
  }


  //---------------------------------------------------------------------------------------------------
  virtual void print() {
    std::list<action*>::iterator i=subactions.begin();
    while (true) {
      (*i)->print();
      ++i;
      if (i==subactions.end())
        break;
      printf(" + ");
    }
  }

protected:
  std::list<action*> subactions;
};


//=====================================================================================================
// вычитание, список
//=====================================================================================================
class sub_action : public add_action {
public:
  //---------------------------------------------------------------------------------------------------
  sub_action(int count, int *brakes_count)
    :add_action(count, brakes_count)
  {
  }

  //---------------------------------------------------------------------------------------------------
  virtual int result() {
    int value=0;
    bool first=true;
    for (std::list<action*>::iterator i=subactions.begin(); i!=subactions.end(); ++i)
      if (first) {
        value=(*i)->result();
        first=false;
      } else
        value-=(*i)->result();
    return value;
  }


  //---------------------------------------------------------------------------------------------------
  virtual void print() {
    std::list<action*>::iterator i=subactions.begin();
    while (true) {
      (*i)->print();
      ++i;
      if (i==subactions.end())
        break;
      printf(" - ");
    }
  }
};

//=====================================================================================================
// умножение, список
//=====================================================================================================
class mul_action : public action {
public:
  //---------------------------------------------------------------------------------------------------
  mul_action(int count, int *brakes_count)
    :left (random_action(count/2, brakes_count, use_actions & ~(USE_MUL | USE_DEV | USE_ADD | USE_SUB)))
    ,right(random_action(count-count/2, brakes_count, use_actions & ~(USE_MUL | USE_DEV | USE_ADD | USE_SUB)))
  {
  }

  //---------------------------------------------------------------------------------------------------
  virtual ~mul_action() {
    delete left;
    delete right;
  }

  //---------------------------------------------------------------------------------------------------
  int get_count() {
    return left->get_count()+ right->get_count();
  }

  //---------------------------------------------------------------------------------------------------
  virtual int result() {
    return left->result()*right->result();
  }

  //---------------------------------------------------------------------------------------------------
  virtual void print() {
    left->print();
    printf(" * ");
    right->print();
  }

protected:
  action *left;
  action *right;
};

//=====================================================================================================
// деление, список
//=====================================================================================================
class dev_action : public action {
public:
  //---------------------------------------------------------------------------------------------------
  dev_action(int count, int *brakes_count)
    :left(NULL)
    ,right(NULL)
  {
    do {
      if (left)
        delete left;
      if (right)
        delete right;
      left=random_action(count/2, brakes_count, use_actions & ~(USE_MUL | USE_DEV | USE_ADD | USE_SUB));
      right=random_action(count-count/2, brakes_count, use_actions & ~(USE_MUL | USE_DEV | USE_ADD | USE_SUB));
    } while (left->result()==right->result() || left->result()<=1 || right->result()<=1 || (left->result() % right->result()));
  }

  //---------------------------------------------------------------------------------------------------
  virtual ~dev_action() {
    delete left;
    delete right;
  }

  //---------------------------------------------------------------------------------------------------
  int get_count() {
    return left->get_count()+ right->get_count();
  }

  //---------------------------------------------------------------------------------------------------
  virtual int result() {
    return left->result()/right->result();
  }

  //---------------------------------------------------------------------------------------------------
  virtual void print() {
    left->print();
    printf(" : ");
    right->print();
  }

protected:
  action *left;
  action *right;
};


//=====================================================================================================
action *random_action(int count, int *brakes_count, int actions_mask) {
  if (count==0)
    return NULL;

  while (count>1) {
    int type=rand()%6;

    if (type==0 && (actions_mask & USE_NUMBER))
      break;

    if (type==1 && (actions_mask & USE_ADD))
      return new add_action(count, brakes_count);

    if (type==2 && (actions_mask & USE_SUB))
      return new sub_action(count, brakes_count);

    if (type==3 && (actions_mask & USE_MUL))
      return new mul_action(count, brakes_count);

    if (type==4 && (actions_mask & USE_DEV))
      return new dev_action(count, brakes_count);

    if (type==5 && *brakes_count>0 && (actions_mask & USE_BRAKES))
      return new brakes_action(count, brakes_count);
  }

  return new number(rand()%(max_value-min_value)+min_value);
}



//=====================================================================================================
// сам генератор
//=====================================================================================================
class generator {
public:
  //---------------------------------------------------------------------------------------------------
  generator()
    :left  (NULL)
    ,right (NULL)
  {
  }

  //---------------------------------------------------------------------------------------------------
  virtual ~generator(){
    clean();
  }

  //---------------------------------------------------------------------------------------------------
  bool init(int argc, const char *argv[]) {
    examples_count=atoi(argv[1]);
    if (examples_count<1)
      return false;

    items_count=atoi(argv[2]);
    if (items_count<3)
      return false;

    if (strchr(argv[3], '+'))
      use_actions|=USE_ADD;

    if (strchr(argv[3], '-'))
      use_actions|=USE_SUB;

    if (strchr(argv[3], '*'))
      use_actions|=USE_MUL;

    if (strchr(argv[3], '/'))
      use_actions|=USE_DEV;

    if (!use_actions)
      return false;
    use_actions|=USE_BRAKES;
    use_actions|=USE_NUMBER;

    max_brakes=atoi(argv[4]);
    if (max_brakes<0)
      max_brakes=0;

    min_value=atoi(argv[5]);
    if (min_value<0)
      min_value=0;

    max_value=atoi(argv[6]);
    if (max_value<=min_value)
      return false;

    return true;
  }

  //---------------------------------------------------------------------------------------------------
  bool generate() {
    int left_count=(items_count-1)/2;
    int right_count=items_count-1-left_count;
    int brakes_count=max_brakes;

    left=random_action(left_count, &brakes_count, use_actions & ~(USE_NUMBER | USE_DEV | USE_MUL));
    right=random_action(right_count, &brakes_count, use_actions & ~(USE_NUMBER | USE_DEV | USE_MUL));

    if (left->result()!=right->result()) {
      if (left->result()<right->result())
        left=new add_action(left, new number(right->result()-left->result()));
      else
        right=new add_action(right, new number(left->result()-right->result()));
    }

    return true;
  }

  //---------------------------------------------------------------------------------------------------
  bool print() {
    left->print();
    printf(" = ");
    right->print();
    printf("\n");
  }

  //---------------------------------------------------------------------------------------------------
  void clean() {
    if (left) {
      delete left;
      left=NULL;
    }

    if (right) {
      delete right;
      right=NULL;
    }
  }

  //---------------------------------------------------------------------------------------------------
  int get_examples_count() {
    return examples_count;
  }

private:
  action *left;
  action *right;
};


//=====================================================================================================
int main(int argc, const char *argv[]){
  printf("Examples generator (c) Alexander Semenov 2016\n");

  if (argc < 6) {
    printf("Usage: examples_generator examples_count items_count {+-*/} max_brakes min_value max_value\n");
    return 0;
  }

  srand(time(NULL));

  generator gen;
  if (!gen.init(argc, argv)) {
    printf("bad params\n");
    return 1;
  }

  for (int i=0; i<gen.get_examples_count(); i++) {
    if (!gen.generate())
      return 1;
    int x=number::set_x();
    printf("%d.) ", i+1);
    gen.print();
 //   printf("\t x=%d\n", x);
    gen.clean();
  }

  printf("done\n");
  return 0;
}