#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cassert>
#include<random>
#include"time.h"
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <sstream>
#include <SDL_mixer.h>
#ifdef _WINDOWS
#define RESOURCE_FOLDER "              "
#else
#define RESOURCE_FOLDER "NYUCodebase/"
#endif
using namespace std;

const float framesPerSecond = 10.0f;

GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
}

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

struct Location{
    int maxX;
    int maxY;
    int width;
    int height;
};
bool moveTo(float elapsed, float& num, float& dest){
    if (dest > num) {
        num += elapsed;
    }else if (dest < num){
        num -= elapsed;
    }
    if (num + 0.02 > dest && num-0.02 < dest) {
        num = dest;
        return true;
    }
    return false;
    
    
}
void setUpHeroSprites(vector<Location>& heroSprites){
    ifstream precel;
    precel.open(RESOURCE_FOLDER"heroSprites.txt");
    string line;
    int pos = 0;
    string x,y,w,h;
    while (getline(precel,line)){
        
        
        pos = line.find(",");
        x = line.substr(0, pos);
        line.erase(line.begin(), line.begin()+pos + 1);
        
        
        pos = line.find(",");
        y = line.substr(0, pos);
        line.erase(line.begin(), line.begin()+pos + 1);
        
        pos = line.find(",");
        w = line.substr(0, pos);
        line.erase(line.begin(), line.begin()+pos + 1);
        
        pos = line.find(",");
        h = line.substr(0, pos);
        line.erase(line.begin(), line.begin()+pos + 1);
        
        Location temp;
        
        temp.maxX = stoi(x);
        temp.maxY = stoi(y);
        temp.width = stoi(w);
        temp.height = stoi(h);
        
        heroSprites.push_back(temp);
    
        
    }
}
    
    




float enforceMax(float num, float max){
    if (num > max){
        return max;
    }else if(num < -max){
        return(-max);
    }
    return num;
    
}

class Entity;

class Ability{
public:
    string name;
    int baseDamage = 10;
    int damage = 15;
    int multiplier = 1;
    string stat = "strength";
    bool AOE = false;
    bool heals = false;
    GLuint toolTip;
    Entity* target;
    Entity* caster;
    bool spell = false;
    Mix_Chunk* sound;
    
};
class Entity{
public:
    string name;
    float readiness = 0;
    bool ready = false;
    int health = 50;
    int maxHealth = 50;
    float x = 0;
    float y = 0;
    float width = 0.1;
    float height = 0.5;
    float vertices[12] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
    float TexCoords[12] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    GLuint texture;
    GLuint currentTexture;
    float xSpeed = 0;
    float ySpeed = 0;
    float xAcceleration = 0;
    float yAcceleration = 0;
    float frictionX = 10;
    float frictionY = 1;
    float maxSpeed = 10;
    int baseIndex = 0;
    int currentIndex = 0;
    int animIndex = 0;
    bool casting = false;
    bool attacking = false;
    bool hurting = false;
    bool bloodied = false;
    bool enemy = true;
    bool dead = false;
    float animationCounter = 0;
    Entity(int givenTexutre, float givenX = 0, float givenY = 0, float givenWidth = 0.1, float givenHeight = 0.5) {texture = givenTexutre, x = givenX, y = givenY, width = givenWidth, height = givenHeight;
        currentTexture = texture;
        
        
    }
    Entity operator=(const Entity& ent){
        return Entity(ent.texture, ent.x, ent.y, ent.width, ent.height);
    }
    virtual void move(float elapsed){
        std::cout <<"oops" << std::endl;
    }
    virtual void hit(){
        //do nothing
    }
    
};

class Selector : public Entity{
public:
    Entity* target;
    float baseY;
    bool up = true;
    Selector() : Entity(LoadTexture(RESOURCE_FOLDER"arrow.png"),0,0,0.1,0.1){}
};


class abilityTooltip : public Entity{
public:
    abilityTooltip(Ability ability):Entity(ability.toolTip, 0, 0, 0.5, 0.5){}
};

bool isColiding(Entity A, Entity B){
    if (A.y+A.height/2 > B.y+B.height/2) {
        return false;
    }
    if (A.y+A.height/2 < B.y-B.height/2) {
        return false;
    }
    if (A.x-A.width/2 > B.x+B.width/2) {
        return false;
    }
    if (A.x+A.width/2 < B.x-B.width/2) {
        return false;
    }
    
    return true;
}



class Player : public Entity{
public:
    vector<Ability> abilities;
    int speedMod = 10;
    int strength = 5;
    int inteligence = 5;
    int dexterity = 5;
    int constitution = 5;
    const int attackAnimationArr[19] = {2,0,2,0,2,0,2,0,0,6,5,6,5,6,5,6,5,0,0};
    const int spellAnimationArr[22] = {0,2,0,2,0,2,0,2,0,0,5,2,5,2,5,2,5,2,1,1,0,0};
    const int hurtAnimationArr[4] = {3,3,3,3};
    const int idleAnimationArr[2] = {0,2};
    
    Player(int givenTexture, float givenX = 0, float givenY = 0, float givenWidth = 0.1, float givenHeight = 0.5): Entity(givenTexture, givenX, givenY, givenWidth,givenHeight){
        enemy = false;
    }
    void setUpAbilities(){
        for (int i = 0; i < abilities.size(); ++i) {
            if (abilities[i].stat == "strength") {
                abilities[i].damage = abilities[i].baseDamage+abilities[i].multiplier*strength;
            }
            if (abilities[i].stat == "dexterity") {
                abilities[i].damage = abilities[i].baseDamage+abilities[i].multiplier*dexterity;
            }
            if (abilities[i].stat == "inteligence") {
                abilities[i].damage = abilities[i].baseDamage+abilities[i].multiplier*inteligence;
            }
            if (abilities[i].stat == "constitution") {
                abilities[i].damage = abilities[i].baseDamage+abilities[i].multiplier*constitution;
            }
        }
    }
    void idleAnimation(float elapsed){
        animationCounter += elapsed;
        if (animationCounter > 1.0/framesPerSecond) {
            animIndex++;
            animationCounter = 0;
        }
        if (animIndex > sizeof(idleAnimationArr)/4 - 1) {
            animIndex = 0;
        }
        currentIndex = baseIndex + idleAnimationArr[animIndex];
    }
    
    void attackAnimation(float elapsed){
        attacking = true;
        animationCounter += elapsed;
        if (animationCounter > 1.0/framesPerSecond) {
            animIndex++;
            animationCounter = 0;
        }
        if (animIndex > sizeof(attackAnimationArr)/4 - 1) {
            animIndex = 0;
            currentIndex = baseIndex;
            attacking = false;
            return;
        }
        currentIndex = baseIndex + attackAnimationArr[animIndex];
    }
    
    void spellAnimation(float elapsed){
        casting = true;
        animationCounter += elapsed;
        if (animationCounter > 1.0/framesPerSecond) {
            animIndex++;
            animationCounter = 0;
        }
        if (animIndex > sizeof(spellAnimationArr)/4 - 1) {
            animIndex = 0;
            currentIndex = baseIndex;
            casting = false;
            return;
        }
        currentIndex = baseIndex + spellAnimationArr[animIndex];
    }
    void hurtingAnimation(float elapsed){
        hurting = true;
        animationCounter += elapsed;
        if (animationCounter > 1.0/framesPerSecond) {
            animIndex++;
            animationCounter = 0;
        }
        if (animIndex > sizeof(hurtAnimationArr)/4 - 1) {
            animIndex = 0;
            currentIndex = baseIndex;
            hurting = false;
            return;
        }
        currentIndex = baseIndex + hurtAnimationArr[animIndex];
    }
    
    
    
};
class Enemy : public Entity{
public:
    int speedMod = 10;
    int strength = 5;
    int inteligence = 5;
    int dexterity = 10;
    int constitution = 5;
    float canCast = false;
    Entity* attackTarget;
    float baseX;
    float baseY;
    vector<GLuint> attackAnimationVec;
    vector<GLuint> castAnimationVec;
    vector<GLuint> idleAnimationVec;
    vector<GLuint> hitAnimationVec;
    vector<Ability> abilities;
    Enemy(int givenTexture, float givenX = 0, float givenY = 0, float givenWidth = 0.1, float givenHeight = 0.5): Entity(givenTexture, givenX, givenY, givenWidth,givenHeight){
        baseX = givenX;
        baseY = givenY;
    }
    
    void setUpAbilities(){
        for (int i = 0; i < abilities.size(); ++i) {
            if (abilities[i].stat == "strength") {
                abilities[i].damage = abilities[i].baseDamage+abilities[i].multiplier*strength;
            }
            if (abilities[i].stat == "dexterity") {
                abilities[i].damage = abilities[i].baseDamage+abilities[i].multiplier*dexterity;
            }
            if (abilities[i].stat == "inteligence") {
                abilities[i].damage = abilities[i].baseDamage+abilities[i].multiplier*inteligence;
            }
            if (abilities[i].stat == "constitution") {
                abilities[i].damage = abilities[i].baseDamage+abilities[i].multiplier*constitution;
            }
        }
    }
    
    virtual void idleAnimation(float elapsed){
        //do nothing
    }
    
    virtual void attackAnimation(float elapsed){
        //do nothing
    }
    
    virtual void spellAnimation(float elapsed){
        //do nothing
    }
    virtual void hitAnimation(float elapsed){
        //do nothing
    }
    
    
    
    
};

class Hench : public Enemy{
public:
    Hench() : Enemy(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"), -2,0,.5,.5){
        Ability roll;
        roll.sound = Mix_LoadWAV("punch.wav");
        abilities.push_back(roll);
        
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack1.png"));
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack2.png"));
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack3.png"));
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack1.png"));
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack2.png"));
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack3.png"));
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack1.png"));
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack2.png"));
        Enemy::attackAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/attack3.png"));
        
        
        
        Enemy::idleAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        
        
        
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/stance.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/hit.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/hit.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/hit.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/hit.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/hit.png"));
        Enemy::hitAnimationVec.push_back(LoadTexture(RESOURCE_FOLDER"enemySprites/Hench/hit.png"));
        
    }
    void idleAnimation(float elapsed){
        moveTo(elapsed*4, x, baseX);
        moveTo(elapsed*4, y, baseY);
        animationCounter += elapsed;
        if (animationCounter > 1.0/framesPerSecond) {
            animIndex++;
            animationCounter = 0;
        }
        if (animIndex > idleAnimationVec.size() - 1) {
            animIndex = 0;
        }
        currentTexture = idleAnimationVec[animIndex];
    }
    
    void attackAnimation(float elapsed){
        attacking = true;
        animationCounter += elapsed;
        moveTo(elapsed*4, x, attackTarget->x);
        moveTo(elapsed*4, y, attackTarget->y);
        if (animationCounter > 1.0/framesPerSecond) {
            animIndex++;
            animationCounter = 0;
        }
        if (animIndex > attackAnimationVec.size() - 1) {
            animIndex = 0;
            attacking = false;
            attackTarget->hurting = true;
            return;
        }
        currentTexture = attackAnimationVec[animIndex];
    }
    
    void spellAnimation(float elapsed){
        casting = true;
        animationCounter += elapsed;
        if (animationCounter > 1.0/framesPerSecond) {
            animIndex++;
            animationCounter = 0;
        }
        if (animIndex > castAnimationVec.size() - 1) {
            animIndex = 0;
            
            casting = false;
            return;
        }
        currentTexture = castAnimationVec[animIndex];
    }
    void hitAnimation(float elapsed){
        hurting = true;
        animationCounter += elapsed;
        if (animationCounter > 1.0/framesPerSecond) {
            animIndex++;
            animationCounter = 0;
        }
        if (animIndex > hitAnimationVec.size() - 1) {
            animIndex = 0;
            
            hurting = false;
            return;
        }
        currentTexture = hitAnimationVec[animIndex];
    }
    
    
    
};

//class Menu : public Entity{
//public:
//    GLuint lose;
//    GLuint win;
//    GLuint base;
//    Menu() : Entity(LoadTexture(RESOURCE_FOLDER"menu.png")){
//        lose = LoadTexture(RESOURCE_FOLDER"lose.png");
//        win = LoadTexture(RESOURCE_FOLDER"win.png");
//        base = LoadTexture(RESOURCE_FOLDER"menu.png");
//    }
//
//};


Enemy* getLowestHealth(vector<Enemy>& enemies){
    Enemy* lowestEnemy = &enemies[0];
    float lowestValue = 1;
    for (int i = 0; i < enemies.size(); ++i) {
        if (enemies[i].health/enemies[i].maxHealth < lowestValue) {
            lowestEnemy = &enemies[i];
        }
    }
    if (lowestValue == 1) {
        return nullptr;
    }
    else{
        return lowestEnemy;
    }
}

Enemy* getLowestHealth(vector<Enemy*>& enemies){
    Enemy* lowestEnemy = enemies[0];
    float lowestValue = 1;
    for (int i = 0; i < enemies.size(); ++i) {
        if (enemies[i]->health/enemies[i]->maxHealth < lowestValue) {
            lowestEnemy = enemies[i];
        }
    }
    if (lowestValue == 1) {
        return nullptr;
    }
    else{
        return lowestEnemy;
    }
}



void drawThings(ShaderProgram* program, Entity& ent){
    glBindTexture(GL_TEXTURE_2D,ent.currentTexture);
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, ent.vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, ent.TexCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}


void DrawSpriteSheetSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY, Entity& ent) {
    float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
    float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
    float spriteWidth = 1.0/(float)spriteCountX;
    float spriteHeight = 1.0/(float)spriteCountY;
    GLfloat texCoords[] = {

        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight,
        u+spriteWidth, v,

        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,

    };

    

    float vertices[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f,
        -0.5f, 0.5f, -0.5f};


    std::copy(std::begin(texCoords), std::end(texCoords), std::begin(ent.TexCoords));


    drawThings(program, ent);
}




void handleEntity(Entity* ent, Matrix modelviewMatrix,ShaderProgram program, float elapsed){
    modelviewMatrix.Identity();
    modelviewMatrix.Translate(ent->x, ent->y, 0.0f);
    modelviewMatrix.Scale(ent->width, ent->height, 1.0);
    program.SetModelviewMatrix(modelviewMatrix);
    
    //drawThings(&program, ent);
}


class gameState{
public:
    vector<Enemy*> enemies;
    vector<Player*> players;
    int state = 0;
    
    
    
    
    
    
};


//void DrawText(ShaderProgram *program, std::string text, float size, Matrix modelviewMatrix, Entity baseLetter) {
//
//    std::vector<Entity> word;
//    for (float i = 0; i < text.size(); i++) {
//        Entity letter(baseLetter.texture, ((3*2)*i/(text.size()-1))-3 ,0, size,size);
//        word.push_back(letter);
//    }
//    for (int i = 0; i < word.size(); ++i) {
//        handleEntity(&word[i], modelviewMatrix, *program, 0);
//        DrawSpriteSheetSprite(program, text[i], 16, 16, word[i]);
//    }
//
//    // draw this data (use the .data() method of std::vector to get pointer to data)
//}

int main(int argc, char *argv[])
{
    
    gameState game;
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    SDL_Window* displayWindow;
    srand(time(NULL));
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    float lastFrameTicks = 0.0f;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    
    glViewport(0, 0, 1280, 720);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(0, 0, 1280, 720);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    Matrix projectionMatrix;
    projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
    Matrix modelviewMatrix;
    SDL_Event event;
    bool done = false;
    
    //glBindTexture(GL_TEXTURE_2D, loneArcher.currentTexture);
    
    //object definitions
    Hench baddie;
    Hench baddie2;
    Hench baddie3;
    Hench otherBaddie;
    
   
    
    baddie.name = "baddie";
    otherBaddie.name = "otherBaddie";
    
    Ability currentAbility;
    vector<Ability> theStack;
    vector<Enemy*> enemies;
    enemies.push_back(&baddie);
    enemies.push_back(&otherBaddie);
    
    
    int additional = rand()%3;
    
    if (additional == 1) {
        enemies.push_back(&baddie2);
    }
    if (additional == 2) {
        enemies.push_back(&baddie2);
        enemies.push_back(&baddie3);
    }
    
    
    
    vector<Player*> players;
    
    //sound definitions
    Mix_Chunk *switchSelect;
    switchSelect = Mix_LoadWAV("switchSelect.wav");
    
    Mix_Chunk *select;
    select = Mix_LoadWAV("select.wav");
    
    Mix_Music *battleMusic;
    battleMusic = Mix_LoadMUS("music.wav");
    
    Mix_Music *menuMusic;
    menuMusic = Mix_LoadMUS("menuMusic.wav");
    
    
    //primitive definitions
    int moveIndex = 0;
    int selectingMode = 0;
    int targetIndex = 0;
    int remainingTargets = 0;
    
    

    vector<abilityTooltip> moveSet1;
    vector<abilityTooltip> moveSet2;
    vector<abilityTooltip> moveSet3;
    vector<abilityTooltip> moveSet4;
    Ability punch, kick, slap;
    Ability fireball, heal, iceKnife;
    Ability bite, maul, claw;
    
    punch.toolTip = LoadTexture(RESOURCE_FOLDER"punch.png");
    kick.toolTip = LoadTexture(RESOURCE_FOLDER"kick.png");
    slap.toolTip = LoadTexture(RESOURCE_FOLDER"slap.png");
    fireball.toolTip = LoadTexture(RESOURCE_FOLDER"fireball.png");
    heal.toolTip = LoadTexture(RESOURCE_FOLDER"lightningBolt.png");
    iceKnife.toolTip = LoadTexture(RESOURCE_FOLDER"iceKnife.png");
    bite.toolTip = LoadTexture(RESOURCE_FOLDER"bite.png");
    maul.toolTip = LoadTexture(RESOURCE_FOLDER"maul.png");
    claw.toolTip = LoadTexture(RESOURCE_FOLDER"claw.png");
    punch.name = "punch";
    kick.name = "kick";
    slap.name = "slap";
    fireball.name = "fireball";
    heal.name = "heal";
    iceKnife.name = "iceKnife";
    bite.name = "bite";
    maul.name = "maul";
    claw.name = "claw";
    heal.heals = true;
    
    fireball.spell = true;
    heal.spell = true;
    iceKnife.spell = true;
    
    Player player1(LoadTexture(RESOURCE_FOLDER"newSheet.png"));
    Player player2(LoadTexture(RESOURCE_FOLDER"newSheet.png"));
    Player player3(LoadTexture(RESOURCE_FOLDER"newSheet.png"));
    
    player1.name = "johnny";
    player2.name = "billy";
    player3.name = "timmy";
    
    player1.baseIndex = 0;
    player2.baseIndex = 7;
    player3.baseIndex = 14;
    
    player1.width = .6;
    player1.height = .75;
    player1.x = 2.5;
    
    player2.width = .6;
    player2.height = .75;
    player2.x = 2.5;
    
    player3.width = .6;
    player3.height = .75;
    player3.x = 2.5;
    
    
    
    punch.sound = Mix_LoadWAV("punch.wav");
    kick.sound = Mix_LoadWAV("punch.wav");
    slap.sound = Mix_LoadWAV("punch.wav");
    
    fireball.sound = Mix_LoadWAV("spell.wav");
    iceKnife.sound = Mix_LoadWAV("spell.wav");
    heal.sound = Mix_LoadWAV("heal.wav");
    
    bite.sound = Mix_LoadWAV("bite.wav");
    claw.sound = Mix_LoadWAV("whoosh.wav");
    maul.sound = Mix_LoadWAV("bite.wav");
    
    
    player1.abilities.push_back(punch);
    player1.abilities.push_back(kick);
    player1.abilities.push_back(slap);
    
    player2.abilities.push_back(fireball);
    player2.abilities.push_back(iceKnife);
    player2.abilities.push_back(heal);
    
    player3.abilities.push_back(bite);
    player3.abilities.push_back(claw);
    player3.abilities.push_back(maul);
    player3.dexterity = 12;
    
    Player menu(LoadTexture(RESOURCE_FOLDER"menu.png"),0,0,7,4);
    players.push_back(&player1);players.push_back(&player2);players.push_back(&player3);
    
    vector<Player*> ready;
    vector<Enemy*> badReady;
    vector<Location> heroSprites;
    setUpHeroSprites(heroSprites);
    
    
    Selector selector;
    
    
    
    
    int counter = 0;
    Mix_PlayMusic(battleMusic, -1);
    
    Entity background(LoadTexture(RESOURCE_FOLDER"background.png"),0,0,7,4);
    unsigned int totalMod = 0;
    bool selection = false;
    while (!done) {
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program.programID);
        program.SetModelviewMatrix(modelviewMatrix);
        program.SetProjectionMatrix(projectionMatrix);
        if (game.state == 0){
            theStack.clear();
            Selector menuSelect;
            menuSelect.baseY = 0;
            
            if (totalMod%4 == 0) {
                selection = true;
            }else{
                selection = false;
            }
            
            if (selection) {
                menuSelect.x = -1;
            }else{
                menuSelect.x = 1;
            }
            
            handleEntity(&menu, modelviewMatrix, program, elapsed);
            drawThings(&program, menu);
            handleEntity(&menuSelect, modelviewMatrix, program, elapsed);
            drawThings(&program, menuSelect);
             SDL_GL_SwapWindow(displayWindow);
            
            for (int i = 0; i < enemies.size(); ++i) {
                float row;
                float column;
                if (i%3 == 0) {
                    row = 0.5;
                }else if(i%3 == 1){
                    row = -.5;
                }else if(i%3 == 2){
                    row = 1.5;
                }
                
                if (i/3 < 1) {
                    column = -1;
                }else if(i/3 < 2){
                    column = -2;
                }else{
                    column = -3;
                }
                
                
                enemies[i]->x = column;
                enemies[i]->y = row;
                enemies[i]->baseX = enemies[i]->x;
                enemies[i]->baseY = enemies[i]->y;
                
            }
            
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                    done = true;
                }if (event.key.keysym.scancode == SDL_SCANCODE_LEFT){
                    totalMod += 1;
                }else if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT){
                    totalMod += 1;
                }else if(event.key.keysym.scancode == SDL_SCANCODE_RETURN){
                    if (selection) {
                        player1.health = player1.maxHealth;
                        player2.health = player2.maxHealth;
                        player3.health = player3.maxHealth;
                        
                        for (int i = 0; i < enemies.size(); ++i) {
                            enemies[i]->health = enemies[i]->maxHealth;
                        }
                        for (int i = 0; i < players.size(); ++i) {
                            players[i]->readiness = rand()%10;
                            players[i]->ready = false;
                        }
                        for (int i = 0; i < enemies.size(); ++i) {
                            enemies[i]->readiness = rand()%10;
                            enemies[i]->ready = false;
                        }
                        
                        
                        game.state =1;
                    }else{
                        exit(-1);
                    }
                
                }
            }
        }else if (game.state == 1){
            
            handleEntity(&background, modelviewMatrix, program, elapsed);
            drawThings(&program, background);
            
            
            //handle the stack
            
            if (theStack.size() > 0) {
                if (theStack.front().target->health < 1) {
                    if (theStack.front().target->enemy) {
                        for (int i = 0; i < enemies.size(); ++i) {
                            if (enemies[i]->health > 1) {
                                theStack.front().target = enemies[i];
                            }
                        }
                    }
                }
                
                if (theStack.front().spell) {
                    theStack.front().caster->casting = true;
                }
                if (!theStack.front().spell) {
                    theStack.front().caster->attacking = true;
                }
            }
            //animate
            for (int i = 0; i < enemies.size(); ++i) {
                if (enemies[i]->attacking) {
                    float dest = 2;
                    moveTo(elapsed, enemies[i]->x, dest);
                    enemies[i]->attackAnimation(elapsed);
                }else if (enemies[i]->casting) {
                    float dest = 2;
                    moveTo(elapsed, enemies[i]->x, dest);
                    enemies[i]->spellAnimation(elapsed);
                }else if (enemies[i]->hurting){
                    enemies[i]->hitAnimation(elapsed);
                }else{
                    if(enemies[i]->health <= 0){
                        enemies.erase(enemies.begin()+i);
                    }
                    enemies[i]->idleAnimation(elapsed);
                }
                
            }
            for (float i = 0; i < players.size(); ++i) {
                players[i]->y = 1.5 - i;
                if (players[i]->attacking) {
                    float dest = 1.75;
                    moveTo(elapsed, players[i]->x, dest);
                    players[i]->attackAnimation(elapsed);
                }else if (players[i]->casting) {
                    float dest = 1.75;
                    moveTo(elapsed, players[i]->x, dest);
                    players[i]->spellAnimation(elapsed);
                }else if (players[i]->hurting){
                    players[i]->hurtingAnimation(elapsed);
                }else{
                    float dest;
                    if (ready.size() > 0) {
                        if (ready.front() == players[i] && selectingMode != 0) {
                            dest = 2.25;
                        }else{
                            dest = 2.5;
                        }
                    }else{
                        dest = 2.5;
                    }
                    
                    
                    moveTo(elapsed, players[i]->x, dest);
                    
                    
                    players[i]->idleAnimation(elapsed);
                }
            }
            
            
            if (theStack.size() > 0) {
                
                
                
                
                if (!theStack.front().caster->casting && !theStack.front().caster->attacking) {
                    
                    
                    
                    
                    if (theStack.front().heals) {
                        if (!theStack.front().AOE) {
                            theStack.front().target->health += theStack.front().damage;
                        }else if(theStack.front().caster->enemy){
                            for (int i = 0; i< enemies.size(); ++i) {
                                enemies[i]->health += theStack.front().damage;
                            }
                        }else if(!theStack.front().caster->enemy){
                            for (int i = 0; i< players.size(); ++i) {
                                players[i]->health += theStack.front().damage;
                            }
                        }
                        
                    }else if(!theStack.front().heals){
                        if (!theStack.front().AOE) {
                            theStack.front().target->health -= theStack.front().damage;
                            theStack.front().target->hurting = true;
                        }else if(theStack.front().caster->enemy){
                            for (int i = 0; i< players.size(); ++i) {
                                players[i]->health -= theStack.front().damage;
                                //players[i]->hurting = true;
                                
                            }
                        }else if(!theStack.front().caster->enemy){
                            for (int i = 0; i< enemies.size(); ++i) {
                                enemies[i]->health -= theStack.front().damage;
                            }
                        }
                    }
                    Mix_PlayChannel(-1, theStack.front().sound, 0);
                    theStack.erase(theStack.begin());
                    //check victory/loss
                    bool lose = true;
                    
                    for (int i = 0; i < players.size(); ++i) {
                        
                        if (players[i]->health > 0) {
                            
                            lose = false;
                        }
                    }
                    if (lose) {
                        cout << "you lose" << endl;
                        game.state = 2;
                    }
                    
                    bool win = true;
                    
                    for (int i = 0; i < enemies.size(); ++i) {
                        
                        if (enemies[i]->health > 0) {
                            
                            win = false;
                        }
                    }
                    if (win) {
                        cout << "you win" << endl;
                        game.state = 3;
                    }
                }
            }
            
            
            //get ready
            
            for (int i = 0; i < players.size(); ++i) {
                if (players[i]->health < 1) {
                    continue;
                }
                if (players[i]->readiness < 100 && !players[i]->ready) {
                    players[i]->readiness += (elapsed)*(players[i]->speedMod+players[i]->dexterity);
                }else if(players[i]->readiness >= 100 && !players[i]->ready){
                    players[i]->ready = true;
                    ready.push_back(players[i]);
                }
            }
            for (int i = 0; i < enemies.size(); ++i) {
                
                if (enemies[i]->readiness < 100 && !enemies[i]->ready) {
                    
                    enemies[i]->readiness += (elapsed)*(enemies[i]->speedMod+enemies[i]->dexterity);
                }else if(enemies[i]->readiness >= 100 && !enemies[i]->ready){
                    enemies[i]->ready = true;
                    badReady.push_back(enemies[i]);
                }
            }
            
            //entity move/draw
            
            
            for (int i = 0; i < players.size(); ++i) {
                if (players[i]->health > 0) {
                    
                
                handleEntity(players[i], modelviewMatrix, program, elapsed);
                DrawSpriteSheetSprite(&program, players[i]->currentIndex, 35, 22, *players[i]);
                }
            }
            for (int i = 0; i < enemies.size(); ++i) {
                handleEntity(enemies[i], modelviewMatrix, program, elapsed);
                drawThings(&program, *enemies[i]);
            }
            
            //render selector
            if (selectingMode == 2) {
                selector.baseY = selector.target->y + selector.target->height/2 + selector.height/2;
                selector.x = selector.target->x;
                handleEntity(&selector, modelviewMatrix, program, elapsed);
                drawThings(&program, selector);
                if (selector.up) {
                    float dest = selector.baseY + 0.1;
                    selector.up = !moveTo(elapsed/4, selector.y, dest);
                }else{
                    float dest = selector.baseY;
                    selector.up = moveTo(elapsed/4, selector.y, dest);
                }
            }
            
            //render ability choices
            if (selectingMode == 1) {
                
                for (int i = 0; i < ready.front()->abilities.size(); ++i) {
                    ready.front()->setUpAbilities();
                    abilityTooltip temp(ready.front()->abilities[i]);
                    if (i == moveIndex) {
                        temp.y = -1.25;
                    }else{
                        temp.y = -1.5;
                    }
                    temp.x = i-1;
                    handleEntity(&temp, modelviewMatrix, program, elapsed);
                    DrawSpriteSheetSprite(&program, 0, 1, 1, temp);
                }
            }
            
            
            while(badReady.size() > 0) {
                bool legal = false;
                Ability currentAbility;
                while (!legal) {
                    legal = true;
                    currentAbility =  badReady.front()->abilities[rand()%badReady.front()->abilities.size()];
                    if (currentAbility.heals){
                        if(!getLowestHealth(enemies)){
                            legal = false;
                            continue;
                        }
                        if(!currentAbility.AOE){
                            currentAbility.target = getLowestHealth(enemies);
                            
                        }
                        
                    }else{
                        if(!currentAbility.AOE){
                            do{
                                int tempIndex = rand()%players.size();
                                currentAbility.target = players[tempIndex];
                                badReady.front()->attackTarget = players[tempIndex];
                            }while(currentAbility.target->health < 1);
                        }
                    }
                }
                badReady.front()->ready = false;
                badReady.front()->readiness = 0;
                currentAbility.caster = badReady.front();
                theStack.push_back(currentAbility);
                badReady.erase(badReady.begin());
            }
            
            
            for (int i = 0; i < enemies.size(); ++i) {
                if (enemies[i]->health > enemies[i]->maxHealth) {
                    enemies[i]->health = enemies[i]->maxHealth;
                }
            }
            for (int i = 0; i < players.size(); ++i) {
                if (players[i]->health > players[i]->maxHealth) {
                    players[i]->health = players[i]->maxHealth;
                }
            }
            
            
            
            //cleanup
            SDL_GL_SwapWindow(displayWindow);
            
            //selectingMode 0 = nothing to select
            //selectingMode 1 = select move
            //selectingMode 2 = select target
            
            if (selectingMode == 0 && ready.size() > 0) {
                selectingMode = 1;
            }
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                    done = true;
                }else if (event.type == SDL_KEYDOWN) {
                    
                    if(event.key.keysym.scancode == SDL_SCANCODE_LSHIFT) {
                        //'cheat' code to make enemy attack
                        baddie.ready = false;
                        baddie.readiness = 99;
                        //debug
                        
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_Q) {
                        for (int i = 0; i < players.size(); ++i) {
                            players[i]->health = 1000;
                            players[i]->maxHealth = 1000;
                            players[i]->strength = 30;
                        }
                    }
                    if (selectingMode == 1) {
                        if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                            Mix_PlayChannel( -1, switchSelect, 0);
                            moveIndex++;
                            if (moveIndex >= ready.front()->abilities.size()) {
                                moveIndex = 0;
                            }
                            
                        }else if(event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                            Mix_PlayChannel( -1, switchSelect, 0);
                            moveIndex--;
                            if (moveIndex < 0) {
                                moveIndex = ready.front()->abilities.size() - 1;
                            }
                        }else if(event.key.keysym.scancode == SDL_SCANCODE_RETURN ) {
                            Mix_PlayChannel( -1, select, 0);
                            currentAbility = ready.front()->abilities[moveIndex];
                            currentAbility.caster = ready.front();
                            
                            if (!currentAbility.AOE) {
                                selectingMode = 2;
                                if (currentAbility.heals) {
                                    selector.x = players[targetIndex]->x;
                                    selector.y = (players[targetIndex]->y)+(players[targetIndex]->height/2) + selector.height/2;
                                    selector.target = players[targetIndex];
                                }else{
                                    handleEntity(enemies[targetIndex], modelviewMatrix, program, elapsed);
                                    selector.x = enemies[targetIndex]->x;
                                    selector.y = (enemies[targetIndex]->y)+(enemies[targetIndex]->height/2) + selector.height/2;
                                    selector.target = enemies[targetIndex];
                                    
                                }
                            }else{
                                selectingMode = 0;
                                theStack.push_back(currentAbility);
                                ready.front()->ready = false;
                                ready.front()->readiness = 0;
                                ready.erase(ready.begin());
                                
                            }
                            moveIndex = 0;
                            
                        }
                    }else if (selectingMode == 2){
                        if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                            Mix_PlayChannel( -1, switchSelect, 0);
                            targetIndex++;
                            if (currentAbility.heals) {
                                if (targetIndex >= players.size()) {
                                    targetIndex = 0;
                                }
                                selector.x = players[targetIndex]->x;
                                selector.y = (players[targetIndex]->y)+(players[targetIndex]->height/2) + selector.height/2;
                                selector.target = players[targetIndex];
                                
                            }else if(!currentAbility.heals){
                                if (targetIndex >= enemies.size()) {
                                    targetIndex = 0;
                                }
                                selector.x = enemies[targetIndex]->x;
                                selector.y = (enemies[targetIndex]->y)+(enemies[targetIndex]->height/2) + selector.height/2;
                                selector.target = enemies[targetIndex];
                            }
                            
                        }else if(event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                            Mix_PlayChannel( -1, switchSelect, 0);
                            targetIndex--;
                            if (currentAbility.heals) {
                                if (targetIndex < 0) {
                                    targetIndex = players.size()-1;
                                }
                                selector.x = players[targetIndex]->x;
                                selector.y = (players[targetIndex]->y)+(players[targetIndex]->height/2) + selector.height/2;
                                selector.target = players[targetIndex];
                            }else if(!currentAbility.heals){
                                if (targetIndex < 0) {
                                    targetIndex = enemies.size()-1;
                                }
                                selector.x = enemies[targetIndex]->x;
                                selector.y = (enemies[targetIndex]->y)+(enemies[targetIndex]->height/2) + selector.height/2;
                                selector.target = enemies[targetIndex];
                            }
                        }else if(event.key.keysym.scancode == SDL_SCANCODE_RETURN ) {
                            Mix_PlayChannel( -1, select, 0);
                            if (currentAbility.heals) {
                                currentAbility.target = players[targetIndex];
                            }else if (!currentAbility.heals){
                                currentAbility.target = enemies[targetIndex];
                                
                            }
                            theStack.push_back(currentAbility);
                            
                            ready.front()->ready = false;
                            ready.front()->readiness = 0;
                            ready.erase(ready.begin());
                            selectingMode = 0;
                            
                        }
                    }
                }
            }
        }else if (game.state == 2){
            menu.currentTexture = LoadTexture(RESOURCE_FOLDER"lose.png");
            game.state = 0;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                    done = true;
                }
        }
        
        }else if (game.state == 3){
            menu.currentTexture = LoadTexture(RESOURCE_FOLDER"win.png");
            game.state = 0;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                    done = true;
                }
            }
        }
        
    }
    SDL_Quit();
    return 0;
}


