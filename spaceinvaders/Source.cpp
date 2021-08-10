#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <memory>

struct Config
{
    float minx = 300;
    float maxx = 1300;
    float miny = 50;
    float maxy = 750;
};
Config config;

class Textures
{
public:
    // player
    sf::Texture* playerTexture;
    sf::Texture* leftEngineTexture;
    sf::Texture* rightEngineTexture;
    sf::Texture* playerLaserTexture;
    sf::Texture* playerMissileTexture;
    sf::Texture* playerShieldTexture;

    // powerups
    sf::Texture* powerupShieldTexture;
    sf::Texture* powerupFireTexture;

    //enemy
    sf::Texture* enemyLaserTexture;
    sf::Texture* enemyTexture;
    sf::Texture* bossTexture;
    sf::Texture* explosionTexture;

    Textures() = default;
    ~Textures()
    {
        //delete this->playerLaserTexture;
        //delete this->playerMissileTexture;
        //delete this->playerShieldTexture;
        //delete this->leftEngineTexture;
        //delete this->rightEngineTexture;
    }

    void init()
    {
        // player
        this->playerTexture = new sf::Texture();
        (*this->playerTexture).loadFromFile("./assets/graphics/playerShip1_blue.png");
        this->leftEngineTexture = new sf::Texture();
        (*this->leftEngineTexture).loadFromFile("./assets/graphics/leftEngine.png");
        this->rightEngineTexture = new sf::Texture();
        (*this->rightEngineTexture).loadFromFile("./assets/graphics/rightEngine.png");
        this->playerLaserTexture = new sf::Texture;
        (*this->playerLaserTexture).loadFromFile("./assets/graphics/laserBlue05.png");
        this->playerMissileTexture = new sf::Texture;
        (*this->playerMissileTexture).loadFromFile("./assets/graphics/missile.png");
        this->playerShieldTexture = new sf::Texture;
        (*this->playerShieldTexture).loadFromFile("./assets/graphics/shield3.png");

        // powerups
        this->powerupShieldTexture = new sf::Texture();
        (*this->powerupShieldTexture).loadFromFile("./assets/graphics/powerup_shield.png");
        this->powerupFireTexture = new sf::Texture();
        (*this->powerupFireTexture).loadFromFile("./assets/graphics/powerup_fire.png");

        // enemy
        this->enemyLaserTexture = new sf::Texture;
        (*this->enemyLaserTexture).loadFromFile("./assets/graphics/laserRed05.png");
        this->enemyTexture = new sf::Texture();
        (*this->enemyTexture).loadFromFile("./assets/graphics/enemyRed3.png");
        this->bossTexture = new sf::Texture();
        (*this->bossTexture).loadFromFile("./assets/graphics/enemyRed5.png");
        this->explosionTexture = new sf::Texture;
        (*this->explosionTexture).loadFromFile("./assets/graphics/explosionSprite.png");
    }
};
Textures globalTextures;

// utility functions
const float pi = 3.141592f;

float norm(sf::Vector2f v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

sf::Vector2f normalize(sf::Vector2f v)
{
    return v / norm(v);
}

sf::Vector2f lerp(sf::Vector2f A, sf::Vector2f B, float t)
{
    return A * (1 - t) + t * B;
}

sf::Vector2f bezier(std::vector<sf::Vector2f> poly, float t)
{
    sf::Vector2f result;
    std::vector<std::vector<sf::Vector2f>> vectors;
    vectors.push_back(poly);
    for (int i = 0; i < poly.size(); i++)
    {
		std::vector<sf::Vector2f> v;
        v.clear();
        for (int j = 0; j < vectors[i].size() - 1; j++)
        {
            v.push_back(lerp(vectors[i][j], vectors[i][j + 1], t));
        }
        vectors.push_back(v);
    }
    return vectors[poly.size()-1][0];
}

sf::Vector2f computeBezierPointDeCasteljau(std::vector<sf::Vector2f> controlPoints, float t)
{
    for (int i = controlPoints.size() -1 ; i > 0 ; i--)
    {
        for (int j = 0; j < i; j++)
        {
            controlPoints[j] = controlPoints[j] * (1 - t) + controlPoints[j + 1] * t;
        }
    }
    return controlPoints[0];
}


class Updatable //abstract class because it has at least one pure virtual method
{
public:
    virtual void update(float dt) = 0; //pure virtual method
};

// game stuff

class Navigation
{
public:
    enum class NavigationStates
    {
        MENU = 0,
        GAME = 1,
        GAME_OVER = 2,
        VICTORY = 3,
        PAUSE = 4
    };
    Navigation::NavigationStates currentState{ Navigation::NavigationStates::MENU };
    float cooldownTimerDuration{ 2.0f };
    float cooldownTimer{ 0.0f };
    bool gameOver{ false };
};
Navigation navigation;

void printIntVector(std::vector<int> v)
{
    for (int i = 0; i < v.size(); i++)
    {
        std::cout << v[i] << ", ";
    }
    std::cout << std::endl;
}

int randomEnemyFire(std::vector<int> matrix, int rows, int columns)
{
    std::vector<int> viableColumns, indexes;
    for (int i = 0; i < columns; i++)
    {
        for (int j = rows - 1; j >= 0; j--)
        {
            if (matrix[j * columns + i] == 1)
            {
                viableColumns.push_back(i);
                indexes.push_back(j * columns + i);
                break;
            }
        }
    }
    int selectedColumn = std::rand() % viableColumns.size();
    int solution = indexes[selectedColumn];
    return solution;
}

sf::VertexArray createVertexArray(std::vector<sf::Vector2f> v, sf::Color color)
{
    sf::VertexArray va;
    va.setPrimitiveType(sf::PrimitiveType::LinesStrip);
    for (auto point : v)
    {
        va.append(sf::Vertex(point, color));
    }
    return va;
}

// -------------------------------
// class definitions
// -------------------------------

class GameEntity : public sf::Drawable, public Updatable
{
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f acceleration;
    sf::Sprite sprite;
    sf::Vector2f size;

    GameEntity()
    {
        ;
    }

    GameEntity(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Texture* texture, sf::Vector2f sizeInWorldSpace)
    {
        this->sprite.setTexture((*texture));
        this->sprite.setOrigin(texture->getSize().x / 2, texture->getSize().y / 2);
        this->sprite.setScale(sizeInWorldSpace.x / texture->getSize().x, sizeInWorldSpace.y / texture->getSize().y);
        this->acceleration = acceleration;
        this->velocity = velocity;
        this->position = position;
        this->size = sizeInWorldSpace;
    }

    ~GameEntity()
    {
        ;
    }

    GameEntity& operator=(const GameEntity& e)
    {
        this->sprite = e.sprite;
        this->acceleration = e.acceleration;
        this->velocity = e.velocity;
        this->position = e.position;

        return *this;
    }

    void virtual update(float dt) override
    {
        this->velocity += this->acceleration * dt;
        this->position += this->velocity * dt;
        this->sprite.setPosition(this->position);
    }

    void changeVelocity(sf::Vector2f v)
    {
        this->velocity = v;
    }

    void changePosition(sf::Vector2f v)
    {
        this->position = v;
        this->sprite.setPosition(this->position);
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(this->sprite);
    }
};

// projectiles

class Projectile : public GameEntity
{
public:
    int damage{ 100 };

    Projectile() = default;

    Projectile(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Texture* texture, sf::Vector2f sizeInWorldSpace, int damage) :
        GameEntity(position, acceleration, velocity, texture, sizeInWorldSpace)
    {
    }

    void virtual update(float dt) override
    {
        this->GameEntity::update(dt);
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        target.draw(this->sprite);
    }
};

class Laser : public Projectile
{
public:

    Laser() = default;

    Laser(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Texture* texture, sf::Vector2f sizeInWorldSpace, int damage):
        Projectile(position, acceleration, velocity, texture, sizeInWorldSpace, damage)
    {
        this->damage = damage;
    }

    void update(float dt)
    {
        this->GameEntity::update(dt);
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(this->sprite);
    }
};

class Missile : public Projectile
{
public:
	int damage{ 200 };
	float speed{ 200.0f };
	std::vector<sf::Vector2f> path;
	float currentTime{ 0.0f };

    Missile() = default;

    Missile(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Texture* texture, sf::Vector2f sizeInWorldSpace, int damage, std::vector<sf::Vector2f> path)
        :Projectile(position, acceleration, velocity, texture, sizeInWorldSpace, damage)
	{
		this->damage = damage;
		this->speed = 200.0f;
		this->currentTime = 0.0f;

		this->path.clear();
		for (int i = 0; i < path.size(); i++)
		{
			this->path.push_back(path[i]);
		}
	}

	void update(float dt)
	{
		float totalTime = std::fabs((config.maxy - config.miny) / this->speed);
		this->currentTime += dt;
		this->changePosition(computeBezierPointDeCasteljau(path, this->currentTime / totalTime));
		if (currentTime > totalTime)
		{
			this->acceleration = { 0 ,0 };
			this->velocity = { this->speed, 0 };
		}
	}

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(this->sprite);
    }
};

// powerups

class Powerup : public GameEntity
{
public:
    enum class PowerupTypes
    {
        SHIELD = 0,
        FIRE = 1
    };
    PowerupTypes type;

    Powerup(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Texture* texture, sf::Vector2f sizeInWorldSpace, PowerupTypes type):
        GameEntity(position, acceleration, velocity, texture, sizeInWorldSpace)
    {
        this->type = type;
    }

    void update(float dt)
    {
        this->GameEntity::update(dt);
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        target.draw(this->sprite);
    }
};

// ===================================
// FIRING PATTERNS
// ===================================

// firing pattern interface

class IFiringPattern
{
public:
    sf::Texture* texture;
    sf::Vector2f size;
    float speed;
    float damage;

    virtual std::vector<Projectile*> fire(GameEntity* actor) = 0;
    virtual ~IFiringPattern() = default;
};

// laser firing patterns
class SingleLaser : public IFiringPattern
{
public:
    SingleLaser(sf::Texture* texture, sf::Vector2f size, float speed, float damage)
    {
        this->texture = texture;
        this->size = size;
        this->speed = speed;
        this->damage = damage;
    }
    std::vector<Projectile*> fire(GameEntity* actor)
    {
        std::vector<Projectile*> v;
        v.push_back(new Laser(actor->position, actor->acceleration, sf::Vector2f({ 0.0f, -this->speed }), this->texture, this->size, this->damage));
        return v;
    }
};

class BurstLaser : public IFiringPattern
{
public:
    BurstLaser(sf::Texture* texture, sf::Vector2f size, float speed, float damage)
    {
        this->texture = texture;
        this->size = size;
        this->speed = speed;
        this->damage = damage;
    }

    std::vector<Projectile*> fire(GameEntity* actor)
    {
        float angle = 5 * pi / 12;
        std::vector<Projectile*> v;
        v.push_back(new Laser(actor->position, actor->acceleration, sf::Vector2f({ 0.0f, -this->speed }), this->texture, this->size, this->damage));
        v.push_back(new Laser(actor->position, actor->acceleration, sf::Vector2f({ this->speed * std::cos(angle), -this->speed * std::sin(angle) }), this->texture, this->size, this->damage));
        v.push_back(new Laser(actor->position, actor->acceleration, sf::Vector2f({ -this->speed * std::cos(angle), -this->speed * std::sin(angle) }), this->texture, this->size, this->damage));
        return v;
    }
};

class MissileCluster : public IFiringPattern
{
public:
    MissileCluster(sf::Texture* texture, sf::Vector2f size, float speed, float damage)
    {
        this->texture = texture;
        this->size = size;
        this->speed = speed;
        this->damage = damage;
    }

	std::vector<Projectile*> fire2(GameEntity* actor)
	{
		std::vector<Projectile*> v;
		std::vector<sf::Vector2f> path;

        // right side
		path.push_back(actor->position);
		path.push_back(actor->position + sf::Vector2f({ 100.0f, 0.0f }));
		path.push_back(actor->position + sf::Vector2f({ 100.0f, -(config.maxy - config.miny) / 3 }));
		path.push_back(actor->position + sf::Vector2f({ -100.0f, -(config.maxy - config.miny) / 3 }));
		path.push_back(actor->position + sf::Vector2f({ -100.0f, -2 * (config.maxy - config.miny) / 3 }));
		path.push_back(actor->position + sf::Vector2f({ 100.0f, -config.maxy }));
		v.push_back(new Missile(actor->position, actor->acceleration, actor->velocity, this->texture, this->size, this->damage, path));

		path.clear();
        path.push_back(actor->position);
		path.push_back(actor->position + sf::Vector2f({ 150.0f, 0.0f }));
		path.push_back(actor->position + sf::Vector2f({ 150.0f, -2 * (config.maxy - config.miny) / 3 }));
		path.push_back(actor->position + sf::Vector2f({ -150.0f, -config.maxy }));
		v.push_back(new Missile(actor->position, actor->acceleration, actor->velocity, this->texture, this->size, this->damage, path));

        // left side
        path.clear();
        path.push_back(actor->position);
        path.push_back(actor->position + sf::Vector2f({ -100.0f, 0.0f }));
        path.push_back(actor->position + sf::Vector2f({ -100.0f, -(config.maxy - config.miny) / 3 }));
        path.push_back(actor->position + sf::Vector2f({ 100.0f, -(config.maxy - config.miny) / 3 }));
        path.push_back(actor->position + sf::Vector2f({ 100.0f, -2 * (config.maxy - config.miny) / 3 }));
        path.push_back(actor->position + sf::Vector2f({ -100.0f, -config.maxy }));
        v.push_back(new Missile(actor->position, actor->acceleration, actor->velocity, this->texture, this->size, this->damage, path));

        path.clear();
        path.push_back(actor->position);
        path.push_back(actor->position + sf::Vector2f({ -150.0f, 0.0f }));
        path.push_back(actor->position + sf::Vector2f({ -150.0f, -2 * (config.maxy - config.miny) / 3 }));
        path.push_back(actor->position + sf::Vector2f({ 150.0f, -config.maxy }));
        v.push_back(new Missile(actor->position, actor->acceleration, actor->velocity, this->texture, this->size, this->damage, path));

		return v;
	}

    std::vector<Projectile*> fire(GameEntity* actor)
    {
        std::vector<Projectile*> v;
        return v;
    }
};

// mount points
class Mountable
{
public:
    IFiringPattern* fp;
};

// =================================
// GAME ENTITIES
// =================================

// player entity

class PlayerShip : public GameEntity
{
public:
    enum FiringPatterns
    {
        LASER_SINGLE = 0,
        LASER_BURST = 1,
        MISSILES = 2
    };
    bool powerupShield{ false };
    bool powerupFire{ false };
    sf::Sprite leftEngine, rightEngine, shieldSprite;
    bool leftEngineActive{ false }, rightEngineActive{ false };
    float playerSpeed{ 400.0f };
    int hp{ 100 };
    int laserDamage{ 100 };
    int missileDamage{ 200 };
    float playerLaserSpeed{ 400.0f };
    float playerMissileSpeed{ 200.0f };
    sf::Vector2f playerLaserSize{ 7.5f, 20.0f };
    sf::Vector2f playerMissileSize{ 10.0f, 25.0f };

    IFiringPattern* firingPattern = nullptr;
    std::map<FiringPatterns, IFiringPattern*> firingPatterns;

    PlayerShip() = default;

    PlayerShip(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Texture* texture, sf::Vector2f sizeInWorldSpace):
        GameEntity(position, acceleration, velocity, texture, sizeInWorldSpace)
    {
        this->powerupShield = false;
        this->powerupFire = false;
        this->hp = 100;

        this->shieldSprite.setTexture(*globalTextures.playerShieldTexture);
        this->shieldSprite.setOrigin((*globalTextures.playerShieldTexture).getSize().x / 2, (*globalTextures.playerShieldTexture).getSize().y / 2);
        this->shieldSprite.setScale(sizeInWorldSpace.x / (*globalTextures.playerShieldTexture).getSize().x, sizeInWorldSpace.y / (*globalTextures.playerShieldTexture).getSize().y);

        this->leftEngine.setTexture(*globalTextures.leftEngineTexture);

        this->rightEngine.setTexture(*globalTextures.rightEngineTexture);

        this->firingPatterns.insert(std::pair<FiringPatterns, IFiringPattern*>(PlayerShip::FiringPatterns::LASER_SINGLE, new SingleLaser(globalTextures.playerLaserTexture, this->playerLaserSize, this->playerLaserSpeed, this->laserDamage)));
        this->firingPatterns.insert(std::pair<FiringPatterns, IFiringPattern*>(PlayerShip::FiringPatterns::LASER_BURST, new BurstLaser(globalTextures.playerLaserTexture, this->playerLaserSize, this->playerLaserSpeed, this->laserDamage)));
    }

    ~PlayerShip()
    {
        delete this->firingPatterns[FiringPatterns::LASER_SINGLE];
        delete this->firingPatterns[FiringPatterns::LASER_BURST];
    }

    PlayerShip& operator=(const PlayerShip& ship)
    {
        this->sprite = ship.sprite;
        this->acceleration = ship.acceleration;
        this->velocity = ship.velocity;
        this->position = ship.position;

        this->powerupShield = false;
        this->powerupFire = false;
        this->hp = 100;

        this->shieldSprite.setTexture(*globalTextures.playerShieldTexture);
        this->shieldSprite.setOrigin((*globalTextures.playerShieldTexture).getSize().x / 2, (*globalTextures.playerShieldTexture).getSize().y / 2);
        this->shieldSprite.setScale(ship.sprite.getLocalBounds().width / (*globalTextures.playerShieldTexture).getSize().x, ship.sprite.getLocalBounds().height / (*globalTextures.playerShieldTexture).getSize().y);

        this->leftEngine.setTexture(*globalTextures.leftEngineTexture);
        this->rightEngine.setTexture(*globalTextures.rightEngineTexture);

        this->firingPatterns.clear();
        this->firingPatterns.insert(std::pair<FiringPatterns, IFiringPattern*>(PlayerShip::FiringPatterns::LASER_SINGLE, new SingleLaser(globalTextures.playerLaserTexture, this->playerLaserSize, this->playerLaserSpeed, this->laserDamage)));
        this->firingPatterns.insert(std::pair<FiringPatterns, IFiringPattern*>(PlayerShip::FiringPatterns::LASER_BURST, new BurstLaser(globalTextures.playerLaserTexture, this->playerLaserSize, this->playerLaserSpeed, this->laserDamage)));

        return *this;
    }

    void update(float dt)
    {
        this->changeVelocity({ 0 ,0 });
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            this->rightEngineActive = true;
            this->changeVelocity({ -this->playerSpeed, 0 });
        }
        else
        {
            this->rightEngineActive = false;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            this->leftEngineActive = true;
            this->changeVelocity({ this->playerSpeed, 0 });
        }
        else
        {
            this->leftEngineActive = false;
        }

        this->GameEntity::update(dt);
        this->leftEngine.setPosition(this->position + sf::Vector2f({ -60.0f, 00.0f }));
        this->rightEngine.setPosition(this->position + sf::Vector2f({ 20.0f, 00.0f }));
        this->shieldSprite.setPosition(this->position);
    }

    int hit(int damage)
    {
        if (this->powerupShield)
        {
            this->powerupShield = false;
            this->powerupFire = false;
        }
        else
        {
            this->hp -= damage;
        }
        return this->hp;
    }

    std::vector<Projectile*> fire(std::vector<Projectile*>& vector)
    {
        if (!this->powerupFire)
        {
            this->firingPattern = this->firingPatterns[FiringPatterns::LASER_SINGLE];
        }
        else
        {
            this->firingPattern = this->firingPatterns[FiringPatterns::LASER_BURST];
        }
        std::vector<Projectile*> projectiles = this->firingPattern->fire(this);
        for (int i = 0; i < projectiles.size(); i++)
        {
            vector.push_back(projectiles[i]);
        }
        return projectiles;
    }

    std::vector<Projectile*> fire2()
    {
        std::unique_ptr<MissileCluster> mc = std::make_unique<MissileCluster>(globalTextures.playerMissileTexture, this->playerMissileSize, this->playerMissileSpeed, this->missileDamage);
        return mc->fire2(this);
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        this->GameEntity::draw(target, states);
        if (this->leftEngineActive)
        {
            target.draw(this->leftEngine);
        }
        if (this->rightEngineActive)
        {
            target.draw(this->rightEngine);
        }
        if (this->powerupShield)
        {
            target.draw(this->shieldSprite);
        }
    }
};

class EnemyShip : public GameEntity
{
public:
    enum class MovementType
    {
        DEFAULT = 0,
        PATH = 1,
        BEZIER = 2
    };
    int index;
    int hp{ 100 };
    int laserDamage{ 100 };
    float laserSpeed{ -400.0f };
    float speed{ 100.0f };
    float minx, maxx;
    sf::Vector2f laserSize{ 7.5f, 20.0f };
    MovementType movement{ MovementType::DEFAULT };
    std::vector<sf::Vector2f> path;
    float currentTime;

    IFiringPattern* firingPattern = nullptr;

    EnemyShip() = default;

    EnemyShip(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Texture* texture, sf::Vector2f sizeInWorldSpace):
        GameEntity(position, acceleration, velocity, texture, sizeInWorldSpace)
    {
        this->minx = position.x - 200;
        this->maxx = position.x + 200;
        this->movement = EnemyShip::MovementType::DEFAULT;
        this->hp = 100;

        this->firingPattern = new SingleLaser(globalTextures.enemyLaserTexture, this->laserSize, this->laserSpeed, this->laserDamage);
    }

    EnemyShip operator=(EnemyShip ship)
    {
        this->position = ship.position;
        this->acceleration = ship.acceleration;
        this->velocity = ship.velocity;
        this->sprite = ship.sprite;
        this->hp = 100;

        this->movement = EnemyShip::MovementType::DEFAULT;

        this->minx = ship.position.x - 200;
        this->maxx = ship.position.x + 200;

        this->firingPattern = new SingleLaser(globalTextures.enemyLaserTexture, this->laserSize, this->laserSpeed, this->laserDamage);
    }

    ~EnemyShip()
    {
        delete this->firingPattern;
    }

    void virtual update(float dt) override
    {
        if (this->movement == MovementType::DEFAULT)
        {
			this->GameEntity::update(dt);
			if (this->position.x < this->minx || this->position.x > this->maxx)
			{
				this->velocity = { -this->velocity.x, this->speed };
				this->acceleration.y = -this->speed;
			}
			if (this->velocity.y < 0)
			{
				this->velocity.y = 0.0f;
				this->acceleration.y = 0.0f;
			}
        }
        else
        {
            float totalTime = std::fabs((this->maxx - this->minx) / this->speed);
            this->currentTime += dt;
            this->changePosition(computeBezierPointDeCasteljau(path, this->currentTime / totalTime));
            if (currentTime > totalTime)
            {
                this->movement = MovementType::DEFAULT;
                this->acceleration = { 0 ,0 };
                this->velocity = { this->speed, 0 };
            }
        }
    }

    void changeMovement(MovementType newMovementType)
    {
        std::vector<sf::Vector2f> path;
        if (this->position.x - this->minx > this->maxx - this->position.x)
        {
            path.push_back(this->position);
            path.push_back({ this->position.x, config.maxy / 2 });
            path.push_back({ this->minx, config.maxy / 2 });
            path.push_back({ this->minx, this->position.y });
        }
        else
        {
            path.push_back(this->position);
            path.push_back({ this->position.x, config.maxy / 2 });
            path.push_back({ this->maxx, config.maxy / 2 });
            path.push_back({ this->maxx, this->position.y });
        }
        this->movement = newMovementType;
        this->path = path;
        this->currentTime = 0.0f;
    }

    int hit(int damage) {
        this->hp -= damage;
        return this->hp;
    }

    std::vector<Projectile*> fire()
    {
        return this->firingPattern->fire(this);
    }
};

class BossShip : public EnemyShip
{
public:
    BossShip(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Texture* texture, sf::Vector2f sizeInWorldSpace):
        EnemyShip(position, acceleration, velocity, texture, sizeInWorldSpace)
    {
        this->hp = 2000;
    }

    BossShip operator=(BossShip ship)
    {
        this->position = ship.position;
        this->acceleration = ship.acceleration;
        this->velocity = ship.velocity;
        this->sprite = ship.sprite;
        this->hp = 2000;
    }

    void virtual update(float dt) override
    {
        this->EnemyShip::update(dt);
    }

    void hit(int damage)
    {
        this->hp -= damage;
    }
};

class Animation : public GameEntity
{
public:
    enum class State
    {
        STOPPED = 0,
        PLAYING = 1,
        PAUSED = 2
    };
    float duration;
    float elapsed;
    float looping;
    float currentLoop;
    sf::Vector2u frameSize;
    State state;

    Animation(sf::Vector2f position, sf::Vector2f acceleration, sf::Vector2f velocity, sf::Vector2f sizeInWorldSpace, float duration, sf::Vector2u frameSize, sf::Texture* texture, Animation::State initialState = Animation::State::PLAYING, float looping = 0):
        GameEntity(position, acceleration, velocity, texture, sizeInWorldSpace)
    {
        this->looping = looping;
        this->currentLoop = 0.0f;
        this->duration = duration;
        this->elapsed = 0.0f;
        this->frameSize = frameSize;
        this->state = initialState;

        this->sprite.setTexture(*texture);
        this->sprite.setOrigin({ frameSize.x / 2.0f, frameSize.y / 2.0f });
        this->sprite.setScale(sizeInWorldSpace.x / frameSize.x, sizeInWorldSpace.y / frameSize.y);
        this->sprite.setTextureRect({ 0, 0, (int)this->frameSize.x, (int)this->frameSize.y });
    }

    void update(float dt)
    {
        this->GameEntity::update(dt);

        if (this->state == Animation::State::PLAYING)
        {
            this->elapsed += dt;
            const sf::Texture* t = this->sprite.getTexture();
            sf::Vector2u tSize = (*t).getSize();
            int totalFrames = tSize.x / this->frameSize.x;
            sf::IntRect tRect;
            int currentFrame = (int)(this->elapsed / this->duration * totalFrames);
            tRect.top = 0;
            tRect.left = currentFrame * this->frameSize.x;
            tRect.width = this->frameSize.x;
            tRect.height = this->frameSize.y;
            this->sprite.setTextureRect(tRect);
        }

        if (this->elapsed >= this->duration)
        {
            if (this->looping == 1)
            {
                this->elapsed = 0.0f;
            }
            else if (this->looping == 0)
            {
                this->state = Animation::State::STOPPED;
            }
            else if (this->currentLoop < this->looping)
            {
                this->currentLoop++;
                this->elapsed = 0.0f;
            }
            else
            {
                this->state = Animation::State::STOPPED;
            }
        }
    }

};

class MenuEntity
{
public:
    bool keyPressed;
    int currentMenu, menuOptions;
    float menuOptionSelected, menuOptionSelectCooldown;

    sf::Texture* startButtonTexture;
    sf::Texture* startButtonSelectedTexture;
    sf::Texture* exitButtonTexture;
    sf::Texture* exitButtonSelectedTexture;
    sf::Texture* menuBackgroundTexture;

    sf::Texture* defeatTexture;
    sf::Texture* victoryTexture;

    sf::Vector2u startButtonTextureSize;
    float startButtonSpriteSize;

    sf::Vector2u exitButtonTextureSize;
    float exitButtonSpriteSize;

    sf::Vector2u menuBackgroundTextureSize;
    float menuBackgroundSpriteSize;

    GameEntity startButton, startButtonSelected, exitButton, exitButtonSelected, menuBackground;
    GameEntity victory, defeat;



    void menu_init()
    {
        this->keyPressed = false;
        this->menuOptions = 2;
        this->currentMenu = 0; // start button
        this->menuOptionSelected = 0.0f;
        this->menuOptionSelectCooldown = 0.25f;

        // menu background texture
        this->menuBackgroundTexture = new sf::Texture();
        (*this->menuBackgroundTexture).loadFromFile("./assets/graphics/menu_background.png");

        this->menuBackgroundTextureSize = this->menuBackgroundTexture->getSize();
        this->menuBackgroundSpriteSize = 200.0f;

        // start button texture
        this->startButtonTexture = new sf::Texture();
        (*this->startButtonTexture).loadFromFile("./assets/graphics/start_button.png");

        this->startButtonSelectedTexture = new sf::Texture();
        (*this->startButtonSelectedTexture).loadFromFile("./assets/graphics/start_button_selected.png");

        this->startButtonTextureSize = this->startButtonTexture->getSize();
        this->startButtonSpriteSize = 190.0f;

        //exit button texture
        this->exitButtonTexture = new sf::Texture();
        (*this->exitButtonTexture).loadFromFile("./assets/graphics/exit_button.png");

        this->exitButtonSelectedTexture = new sf::Texture();
        (*this->exitButtonSelectedTexture).loadFromFile("./assets/graphics/exit_button_selected.png");

        this->exitButtonTextureSize = this->exitButtonTexture->getSize();
        this->exitButtonSpriteSize = 190.0f;

        float menux = config.minx + (config.maxx - config.minx) / 3;
        float menuy = config.miny + (config.maxy - config.miny) / 2;

        // menu background sprite
        this->menuBackground.sprite.setTexture(*this->menuBackgroundTexture);
        this->menuBackground.sprite.setScale(this->menuBackgroundSpriteSize / this->menuBackgroundTextureSize.x, this->menuBackgroundSpriteSize * this->menuBackgroundTextureSize.y / this->menuBackgroundTextureSize.x / this->menuBackgroundTextureSize.y);
        this->menuBackground.changePosition({ menux, menuy });

        // start button sprite
        this->startButton.sprite.setTexture(*this->startButtonTexture);
        this->startButton.sprite.setScale(this->startButtonSpriteSize / this->startButtonTextureSize.x, this->startButtonSpriteSize * this->startButtonTextureSize.y / this->startButtonTextureSize.x / this->startButtonTextureSize.y);
        this->startButton.changePosition({ menux + 5, menuy + 125 });

        this->startButtonSelected.sprite.setTexture(*this->startButtonSelectedTexture);
        this->startButtonSelected.sprite.setScale(this->startButtonSpriteSize / this->startButtonTextureSize.x, this->startButtonSpriteSize * this->startButtonTextureSize.y / this->startButtonTextureSize.x / this->startButtonTextureSize.y);
        this->startButtonSelected.changePosition({ menux + 5, menuy + 125 });

        // exit button sprite
        this->exitButton.sprite.setTexture(*this->exitButtonTexture);
        this->exitButton.sprite.setScale(this->exitButtonSpriteSize / this->exitButtonTextureSize.x, this->exitButtonSpriteSize * this->exitButtonTextureSize.y / this->exitButtonTextureSize.x / this->exitButtonTextureSize.y);
        this->exitButton.changePosition({ menux + 5, menuy + 175 });

        this->exitButtonSelected.sprite.setTexture(*this->exitButtonSelectedTexture);
        this->exitButtonSelected.sprite.setScale(this->exitButtonSpriteSize / this->exitButtonTextureSize.x, this->exitButtonSpriteSize * this->exitButtonTextureSize.y / this->exitButtonTextureSize.x / this->exitButtonTextureSize.y);
        this->exitButtonSelected.changePosition({ menux + 5, menuy + 175 });

        // victory texture and sprite
        this->victoryTexture = new sf::Texture();
        (*this->victoryTexture).loadFromFile("./assets/graphics/victory.png");

        this->victory.sprite.setTexture(*this->victoryTexture);
        this->victory.changePosition({ menux, menuy });

        // defeat texture and sprite
        this->defeatTexture = new sf::Texture();
        (*this->defeatTexture).loadFromFile("./assets/graphics/defeat.png");

        this->defeat.sprite.setTexture(*this->defeatTexture);
        this->defeat.changePosition({ menux , menuy });

    }

    void menu_loop(float dt, sf::RenderWindow& window)
    {
        if (this->menuOptionSelected != 0.0f)
        {
            this->menuOptionSelected -= dt;
            if (this->menuOptionSelected < 0.0f)
            {
                this->menuOptionSelected = 0.0f;
            }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        {
            if (this->menuOptionSelected == 0.0f)
            {
                this->currentMenu = (this->currentMenu + 1) % this->menuOptions;
                this->menuOptionSelected = this->menuOptionSelectCooldown;
            }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter))
        {
            if (this->currentMenu == 0 && navigation.currentState == Navigation::NavigationStates::MENU)
            {
                navigation.currentState = Navigation::NavigationStates::GAME;
            }
            else
            {
                window.close();
            }
        }

        // draw
        window.clear();
        window.draw(this->menuBackground.sprite);
        if (this->currentMenu == 0)
        {
            window.draw(this->startButtonSelected.sprite);
        }
        else
        {
            window.draw(this->startButton.sprite);
        }

        if (this->currentMenu == 1)
        {
            window.draw(this->exitButtonSelected.sprite);
        }
        else
        {
            window.draw(this->exitButton.sprite);
        }
    }

    void game_over_loop(float dt, sf::RenderWindow& window)
    {
        if (navigation.cooldownTimer > 0.0f)
        {
            navigation.cooldownTimer -= dt;
        }
        else if (this->keyPressed == true)
        {
            window.clear();
            this->menu_init();
            navigation.currentState = Navigation::NavigationStates::MENU;
            return;
        }
        window.draw(this->defeat.sprite);
    }

    void victory_loop(float dt, sf::RenderWindow& window)
    {
        if (navigation.cooldownTimer > 0.0f)
        {
            navigation.cooldownTimer -= dt;
        }
        else if (this->keyPressed == true)
        {
			window.clear();
            navigation.currentState = Navigation::NavigationStates::MENU;
            this->menu_init();
            return;
        }
        window.draw(this->victory.sprite);
    }

};



EnemyShip* randomEnemyFireImproved(std::vector<EnemyShip*> ships, int rows, int columns)
{
    std::vector<EnemyShip*> viable;

    for (int i = 0; i < ships.size(); i++)
    {
        bool friendlyFire = false;
        for (int j = 0; j < ships.size(); j++)
        {
            if (i != j)
            {
                if (
                    ships[i]->position.x > ships[j]->sprite.getGlobalBounds().left
                    && ships[i]->position.x < ships[j]->sprite.getGlobalBounds().left + ships[j]->sprite.getGlobalBounds().width
                    && ships[i]->position.y < ships[j]->sprite.getGlobalBounds().top
                    )
                {
                    friendlyFire = true;
                    break;
                }
            }
        }
        if (!friendlyFire) viable.push_back(ships[i]);
    }

    int select = std::rand() % viable.size();
    return viable[select];

}

class Game
{
public:

    // text
    sf::Font font;
    sf::Text textEnemies;
    sf::Text textScore;
    int score, scorePerKill;
    sf::Texture* scoreAnimationTexture;

    // game area boundaries
    float minx, maxx, miny, maxy;
    std::vector<sf::Vector2f> boundaries;
    sf::VertexArray box;

    sf::Vector2i mousePos;
    sf::Vector2f mousePosWorld;

    bool lPressed, rPressed, uPressed;
    float rateOfFire, laserCooldown;
    float enemyRateOfFire, enemyLaserCooldown;
    bool changeDirection;
    bool debugEnabled;

    // background
    sf::Texture* backgroundTexture;
    sf::Texture* backgroundTexture2;
    sf::Vector2u backgroundSize;
    float backgroundSpriteSize;
    sf::Sprite backgroundSprite;
    sf::Sprite backgroundSprite2;
    sf::Vector2i backgroundVelocity;
    sf::Vector2f backgroundDefaultPosition;
    sf::Vector2i backgroundTextureSize;
    sf::Vector2i backgroundTexturePosition;
    sf::Vector2f backgroundTexturePositionFloat;
    sf::Vector2i backgroundTexturePosition2;
    sf::Vector2f backgroundTexturePositionFloat2;

    std::vector<sf::Vector2f> backgroundStars;
    float backgroundStarsSpeed;
    float backgroundStarsAmount;

    // player
    sf::Texture* playerTexture;
    PlayerShip playerShip;
    sf::Vector2u playerSize;
    float playerSpriteSize;
    float playerSpeed;

    // power ups
    std::vector<int> powerupIndexes;
    std::vector<Powerup> powerups;
    sf::Texture* powerupShieldTexture;
    sf::Texture* powerupFireTexture;

    // enemy
    sf::Texture* enemyTexture;
    std::vector<EnemyShip*> enemyShips;
    sf::Vector2f enemySize;
    float enemySpriteSize;
    float enemySpeed;
    std::vector<int> enemyBonusIndexes;
    int enemyBonusIndex;
    sf::Texture* bossTexture;
    bool bossActive;

    // player laser
    sf::Texture* playerLaserTexture;
    std::vector<Projectile*> playerLasers;
    std::vector<Missile> playerMissiles;
    sf::Vector2f playerLaserSize;
    float playerLaserSpriteSize;
    float playerLaserSpeed;

    // enemy laser
    std::vector<int> enemyLaserIndexes;
    sf::Texture* enemyLaserTexture;
    std::vector<Projectile*> enemyLasers;
    sf::Vector2f enemyLaserSize;
    float enemyLaserSpriteSize;
    float enemyLaserSpeed;

    // explosion
    sf::Texture* explosionTexture;

    std::vector<sf::Texture*> explosionTextures;
    std::vector<sf::Sprite> explosionSprites;
    sf::Vector2u explosionTextureSize;
    float explosionSpriteSize;

    //std::vector<AnimationEntity> animations;
    std::vector<Animation> animations;

    // sounds
    float masterVolume = 10.0f;
    sf::SoundBuffer* playerLaserBuffer;
    sf::Sound playerLaserSound;
    sf::SoundBuffer* playerMissileBuffer;
    sf::Sound playerMissileSound;
    sf::SoundBuffer* enemyLaserBuffer;
    sf::Sound enemyLaserSound;
    sf::SoundBuffer* enemyExplosionBuffer;
    sf::Sound enemyExplosionSound;

    ~Game()
    {
        delete this->playerTexture;
        delete this->enemyTexture;
        delete this->playerLaserTexture;
        delete this->enemyLaserTexture;
        delete this->backgroundTexture;
        delete this->backgroundTexture2;
        delete this->explosionTexture;
    }
    void game_init()
    {
        // text
		this->font.loadFromFile("./Roboto-Bold.ttf");
        this->textEnemies.setFont(this->font);
        this->textEnemies.setCharacterSize(36);
        this->textEnemies.setFillColor(sf::Color::Cyan);
        this->textEnemies.setStyle(sf::Text::Bold);
        this->textEnemies.setPosition({ 50, 100 });

        this->textScore.setFont(this->font);
        this->textScore.setCharacterSize(36);
        this->textScore.setFillColor(sf::Color::Cyan);
        this->textScore.setStyle(sf::Text::Bold);
        this->textScore.setPosition({ 50, 50 });

        this->score = 0;
        this->scorePerKill = 100;

        this->scoreAnimationTexture = new sf::Texture();
        (*this->scoreAnimationTexture).loadFromFile("./assets/graphics/score_animation.png");

        // clear vectors
        this->enemyShips.clear();
        this->playerLasers.clear();
        this->playerMissiles.clear();
        this->enemyLasers.clear();
        this->animations.clear();
        this->powerups.clear();

        // boundaries
        this->minx = config.minx;
        this->maxx = config.maxx;
        this->miny = config.miny;
        this->maxy = config.maxy;
        this->boundaries.clear();
        this->boundaries.push_back(sf::Vector2f(this->minx, this->miny));
        this->boundaries.push_back(sf::Vector2f(this->maxx, this->miny));
        this->boundaries.push_back(sf::Vector2f(this->maxx, this->maxy));
        this->boundaries.push_back(sf::Vector2f(this->minx, this->maxy));
        this->boundaries.push_back(sf::Vector2f(this->minx, this->miny));
        this->box = createVertexArray(this->boundaries, sf::Color::Cyan);

        // utility vars and flags
        this->debugEnabled = false;
        this->lPressed = false;
        this->rPressed = false;
        this->uPressed = false;
        this->changeDirection = false;
        this->rateOfFire = 0.25f;
        this->laserCooldown = 0.0f;
        this->enemyRateOfFire = 0.5f;
        this->enemyLaserCooldown = 0.0f;

        // background
        this->backgroundDefaultPosition = { this->minx, this->miny };
        this->backgroundTextureSize = { (int)(this->maxx - this->minx), (int)(this->maxy - this->miny) };
        this->backgroundTexturePosition = { 0 ,0 };
        this->backgroundTexturePositionFloat = { 0.0f, 0.0f };
        this->backgroundTexturePosition2 = { 64 ,0 };
        this->backgroundTexturePositionFloat2 = { 64.0f, 0.0f };
        this->backgroundVelocity = { 0, 10 };

        this->backgroundTexture = new sf::Texture();
        (*this->backgroundTexture).loadFromFile("./assets/graphics/black2.png");
        this->backgroundSize = this->backgroundTexture->getSize();
        this->backgroundTexture->setRepeated(true);
        this->backgroundSpriteSize = this->backgroundSize.y;
        this->backgroundSprite.setTexture(*this->backgroundTexture);
        this->backgroundSprite.setTextureRect(sf::IntRect(0, 0, this->maxx - this->minx, this->maxy - this->miny));
        this->backgroundSprite.setPosition(this->backgroundDefaultPosition);

        this->backgroundTexture2 = new sf::Texture();
        (*this->backgroundTexture2).loadFromFile("./assets/graphics/background.png");
        this->backgroundTexture2->setRepeated(true);
        this->backgroundSprite2.setTexture(*this->backgroundTexture2);
        this->backgroundSprite2.setTextureRect(sf::IntRect(0, 0, this->maxx - this->minx, this->maxy - this->miny));
        this->backgroundSprite2.setPosition(this->backgroundDefaultPosition);

        this->backgroundStarsAmount = 50;
        for (int i = 0; i < this->backgroundStarsAmount; i++)
        {
            sf::Vector2f v;
            v.x = this->minx + std::rand() % (int)(this->maxx - this->minx);
            v.y = this->miny + std::rand() % (int)(this->maxx - this->miny);
            this->backgroundStars.push_back(v);
        }
        backgroundStarsSpeed = 100;

        // player entity
        this->playerSpeed = 400.0f;
        this->playerTexture = globalTextures.playerTexture;
        this->playerShip = PlayerShip(sf::Vector2f(this->minx + (this->maxx - this->minx) / 2.0f, this->maxy - 50.0f), { 0,0 }, { 0,0 }, this->playerTexture, { 50, 50 });
        this->playerShip.changePosition(sf::Vector2f(this->minx + (this->maxx - this->minx) / 2.0f, this->maxy - 50.0f));

        // powerup
        this->powerupIndexes = std::vector<int>({ 17, 9, 1 });
        this->powerupShieldTexture = globalTextures.powerupShieldTexture;

        this->powerupFireTexture = globalTextures.powerupFireTexture;

        // enemy

        this->enemyTexture = globalTextures.enemyTexture;
        this->bossTexture = globalTextures.bossTexture;
        this->enemySize = { 50, 40 };
        this->enemySpriteSize = 50.0f;
        this->enemySpeed = 100.0f;
        int totalEnemyShips{ 24 };
        for (int i = 0; i < totalEnemyShips; i++)
        {

            EnemyShip* ship = new EnemyShip({ this->minx + 200 + (float)(i % 6) * this->enemySpriteSize * 2.0f + this->enemySpriteSize / 2, this->miny + (float)(i / 6) * 50 + 20 }, { 0,0 }, { this->enemySpeed, 0 }, this->enemyTexture, this->enemySize);

            ship->index = i;
            this->enemyShips.push_back(ship);
        }
        this->enemyBonusIndexes = std::vector<int>({ 19, 13, 7 });
        this->enemyBonusIndex = -1;
        this->bossActive = false;

        // player lasers
        this->playerLaserTexture = globalTextures.playerLaserTexture;
        this->playerLaserSpriteSize = 10.0f;
        this->playerLaserSpeed = 400.0f;

        // enemy lasers
        this->enemyLaserTexture = globalTextures.enemyLaserTexture;
        this->enemyLaserSize = (sf::Vector2f)(*this->enemyLaserTexture).getSize();
        this->enemyLaserSpriteSize = 10.0f;
        this->enemyLaserSpeed = 400.0f;

        // explosion animation
        this->explosionTexture = globalTextures.explosionTexture;

        // sounds
        this->playerLaserBuffer = new sf::SoundBuffer();
        (*this->playerLaserBuffer).loadFromFile("./assets/sound/laserSmall_000.ogg");
        this->playerLaserSound.setBuffer(*this->playerLaserBuffer);
        this->playerLaserSound.setVolume(this->masterVolume);

        this->playerMissileBuffer = new sf::SoundBuffer();
        (*this->playerMissileBuffer).loadFromFile("./assets/sound/missiles.ogg");
        this->playerMissileSound.setBuffer(*this->playerMissileBuffer);
        this->playerMissileSound.setVolume(this->masterVolume);

        this->enemyLaserBuffer = new sf::SoundBuffer();
        (*this->enemyLaserBuffer).loadFromFile("./assets/sound/laserSmall_001.ogg");
        this->enemyLaserSound.setBuffer(*this->enemyLaserBuffer);
        this->enemyLaserSound.setVolume(this->masterVolume);

        this->enemyExplosionBuffer = new sf::SoundBuffer();
        (*this->enemyExplosionBuffer).loadFromFile("./assets/sound/explosionCrunch_000.ogg");
        this->enemyExplosionSound.setBuffer(*this->enemyExplosionBuffer);
        this->enemyExplosionSound.setVolume(this->masterVolume);
    }

    void game_loop(float dt, sf::RenderWindow& window)
    {
        // check inputs
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            this->lPressed = true;
        }
        else
        {
            this->lPressed = false;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            this->rPressed = true;
        }
        else
        {
            this->rPressed = false;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1))
        {
            this->debugEnabled = !this->debugEnabled;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F2))
        {
            this->playerShip.powerupShield = true;
            this->playerShip.powerupFire = true;
        }

        // state change for ship is done in playerShipUpdate()

        // collision with world boundary
        if (this->playerShip.sprite.getGlobalBounds().left < this->minx)
        {
            this->playerShip.changePosition({ this->minx + this->playerShip.sprite.getGlobalBounds().width / 2, this->playerShip.position.y });
        }
        if (this->playerShip.sprite.getGlobalBounds().left + this->playerShip.sprite.getGlobalBounds().width > this->maxx)
        {
            this->playerShip.changePosition({ this->maxx - this->playerShip.sprite.getGlobalBounds().width / 2, this->playerShip.position.y });
        }

        // IMMA FIRING MAH LAZOR
        if (this->laserCooldown != 0.0f)
        {
            this->laserCooldown -= dt;
            if (this->laserCooldown < 0.0f)
            {
                this->laserCooldown = 0.0f;
            }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        {
            if (this->laserCooldown == 0.0f)
            {
                this->laserCooldown = this->rateOfFire;

                //std::vector<Projectile*> lasers = this->playerShip.fire();
                //for (int i = 0; i < lasers.size(); i++)
                //{
                //    this->playerLasers.push_back(lasers[i]);
                //}
                this->playerShip.fire(this->playerLasers);

                this->playerLaserSound.play();
            }
        }
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
		{
			if (this->laserCooldown == 0.0f)
			{
				this->laserCooldown = this->rateOfFire;
				std::vector<Projectile*> missiles = this->playerShip.fire2();
				for (int i = 0; i < missiles.size(); i++)
				{
					this->playerLasers.push_back(missiles[i]);
				}
				this->playerMissileSound.play();
			}
		}
        
        // checking player laser collision
        std::vector<int> lasers;
        for (int i = 0; i < this->playerLasers.size(); i++)
        {
            for (int j = 0; j < this->enemyShips.size(); j++)
            {
                if (this->enemyShips[j]->sprite.getGlobalBounds().contains(this->playerLasers[i]->position))
                {
                    this->enemyShips[j]->hit(this->playerLasers[i]->damage);
                    this->playerLasers.erase(this->playerLasers.begin() + i);
                    i--;
                    break;
                }
            }
        }
        for (int i = 0; i < this->playerMissiles.size(); i++)
        {
            for (int j = 0; j < this->enemyShips.size(); j++)
            {
                if (this->enemyShips[j]->sprite.getGlobalBounds().contains(this->playerMissiles[i].position))
                {
                    this->enemyShips[j]->hit(this->playerMissiles[i].damage);
                    this->playerMissiles.erase(this->playerMissiles.begin() + i);
                    i--;
                    break;
                }
            }
        }
        // checking dead enemy ships
        for (int i = 0; i < this->enemyShips.size(); i++)
        {
            if (this->enemyShips[i]->hp <= 0)
            {
                Animation animExplosion(this->enemyShips[i]->position, { 0, 0 }, { 0, 0 }, { 50, 50 }, 0.2f, { 50, 50 }, this->explosionTexture);

                Animation animScore((this->enemyShips[i]->position + sf::Vector2f({20, -20})), { 0,100 }, { 30,-100 }, { 40, 20 }, 0.5f, { 40, 20 }, this->scoreAnimationTexture, Animation::State::PLAYING, 5);

                this->score += this->scorePerKill;

                this->animations.push_back(animExplosion);
                this->animations.push_back(animScore);

                this->enemyExplosionSound.play();

                this->enemyShips.erase(this->enemyShips.begin() + i);
                i--;

                // adding boss enemy
                if (this->enemyShips.size() == 0 && !this->bossActive)
                {
                    BossShip* boss = new BossShip(sf::Vector2f({config.minx + (config.maxx - config.minx) / 2, config.miny + 100 }), { 0, 0 }, { 400, 0 }, this->bossTexture, { 150, 100 });
                    this->enemyShips.push_back(boss);
                    this->bossActive = true;
                }
            }
        }
        // out of bounds;
        for (int i = 0; i < this->playerLasers.size(); i++)
        {
            if (this->playerLasers[i]->position.y + this->playerLasers[i]->velocity.y * dt - this->playerLaserSpriteSize * this->playerLaserSize.y / this->playerLaserSize.x / this->playerLaserSize.y < this->miny
                || this->playerLasers[i]->position.x < config.minx
                || this->playerLasers[i]->position.x > config.maxx)
            {
                this->playerLasers.erase(this->playerLasers.begin() + i);
                i--;
            }
        }
        for (int i = 0; i < this->playerMissiles.size(); i++)
        {
            if (this->playerMissiles[i].position.y + this->playerMissiles[i].velocity.y < this->miny
                || this->playerMissiles[i].position.x < config.minx
                || this->playerMissiles[i].position.x > config.maxx)
            {
                this->playerMissiles.erase(this->playerMissiles.begin() + i);
                i--;
            }
        }

        // powerups
        for (int i = 0; i < this->powerups.size(); i++)
        {
            // out of bounds
            if (this->powerups[i].position.y > this->maxy)
            {
                this->powerups.erase(this->powerups.begin() + i);
                i--;
            }
        }
        for (int i = 0; i < this->powerups.size(); i++)
        {
            if (this->playerShip.sprite.getGlobalBounds().contains(powerups[i].position))
            {
                if (this->playerShip.powerupShield)
                {
                    this->playerShip.powerupFire = true;
                }
                this->playerShip.powerupShield = true;
                this->powerups.erase(this->powerups.begin() + i);
                i--;
            }
        }

        // enemy lasers
        if (this->enemyLaserCooldown != 0.0f)
        {
            this->enemyLaserCooldown -= dt;
            if (this->enemyLaserCooldown < 0.0f)
            {
                this->enemyLaserCooldown = 0.0f;
            }
        }
        if (this->enemyLaserCooldown == 0.0f && this->enemyShips.size() > 0)
        {
            this->enemyLaserCooldown = this->enemyRateOfFire;
            this->enemyLasers.push_back(randomEnemyFireImproved(this->enemyShips, 4, 6)->fire()[0]);

            if (this->enemyBonusIndex != -1)
            {
				for (int i = 0; i < this->enemyShips.size(); i++)
				{
                    if (this->enemyShips[i]->index == this->enemyBonusIndex)
                    {
                        this->enemyLasers.push_back(this->enemyShips[i]->fire()[0]);
                    }
				}
            }

            this->enemyLaserSound.play();
        }
        // checking enemy laser collision
        for (int i = 0; i < this->enemyLasers.size(); i++)
        {
            // out of bounds;
            if ((*this->enemyLasers[i]).position.y + (*this->enemyLasers[i]).velocity.y * dt - this->enemyLaserSpriteSize * this->enemyLaserSize.y / this->enemyLaserSize.x / this->enemyLaserSize.y > this->maxy)
            {
                this->enemyLasers.erase(this->enemyLasers.begin() + i);
                i--;
            }
        }
        for (int i = 0; i < this->enemyLasers.size(); i++)
        {
            //if (this->playerShip.sprite.getGlobalBounds().intersects(this->enemyLasers[i].sprite.getGlobalBounds()))
            if (this->playerShip.sprite.getGlobalBounds().contains(this->enemyLasers[i]->position))
            {
                this->playerShip.hit(this->enemyLasers[i]->damage);
                this->enemyLasers.erase(this->enemyLasers.begin() + i);
                i--;
            }
        }

        // movement

        //GameEntity* pge = nullptr;
        //if (rand() < 7)
        //    pge = new PlayerShip;
        //else
        //    pge = new EnemyShip;
        //pge->update(dt); //with virtual the correct method is called. not the pointer type
        this->playerShip.update(dt);

        for (int i = 0; i < this->enemyShips.size(); i++)
        {
            this->enemyShips[i]->update(dt);
        }

        for (int i = 0; i < this->playerLasers.size(); i++)
        {
            this->playerLasers[i]->update(dt);
        }

        for (int i = 0; i < this->playerMissiles.size(); i++)
        {
            this->playerMissiles[i].update(dt);
        }

        for (int i = 0; i < this->enemyLasers.size(); i++)
        {
            this->enemyLasers[i]->update(dt);
        }
        for (int i = 0; i < this->powerups.size(); i++)
        {
            this->powerups[i].update(dt);
        }

        // animation
        for (int i = 0; i < this->animations.size(); i++)
        {
            this->animations[i].update(dt);
        }
        for (int i = 0; i < this->animations.size(); i++)
        {
            if (this->animations[i].state == Animation::State::STOPPED)
            {
                this->animations.erase(this->animations.begin() + i);
                i--;
            }
        }

        // debug victory trigger
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::BackSpace))
        {
            this->enemyShips.clear();
        }

        // check victory condition
        if (this->enemyShips.size() == 0)
        {
            navigation.currentState = Navigation::NavigationStates::VICTORY;
            navigation.cooldownTimer = navigation.cooldownTimerDuration;
            navigation.gameOver = true;
            this->bossActive = false;
            return;
        }
        // check defeat condition
        if (this->playerShip.hp <= 0)
        {
            navigation.currentState = Navigation::NavigationStates::GAME_OVER;
            navigation.cooldownTimer = navigation.cooldownTimerDuration;
            navigation.gameOver = true;
            return;
        }
        for (int i = 0; i < this->enemyShips.size(); i++)
        {
            if (this->enemyShips[i]->position.y > config.maxy)
            {
                navigation.currentState = Navigation::NavigationStates::GAME_OVER;
                navigation.cooldownTimer = navigation.cooldownTimerDuration;
                navigation.gameOver = true;
                return;
            }
        }
        // check powerup condition
        if (this->powerupIndexes.size() != 0)
        {
            if (this->enemyShips.size() == this->powerupIndexes[0])
            {
                Powerup::PowerupTypes type{ Powerup::PowerupTypes::SHIELD };
                sf::Texture* t = this->powerupShieldTexture;
                if (this->playerShip.powerupShield)
                {
                    type = Powerup::PowerupTypes::FIRE;
                    t = this->powerupFireTexture;
                }
                EnemyShip* e = randomEnemyFireImproved(this->enemyShips, 6, 4);
                Powerup p(e->position, sf::Vector2f({ 0, 100 }), sf::Vector2f({ 0, 100 }), t, sf::Vector2f({ 30, 30 }), type);
                this->powerups.push_back(p);
                this->powerupIndexes.erase(this->powerupIndexes.begin());
            }
        }
        if (this->enemyBonusIndexes.size() != 0)
        {
            if (this->enemyShips.size() == this->enemyBonusIndexes[0])
            {
                EnemyShip* e = randomEnemyFireImproved(this->enemyShips, 6, 4);
                this->enemyBonusIndex = e->index;
                for (int i = 0; i < this->enemyShips.size(); i++)
                {
                    if (this->enemyShips[i]->index == e->index)
                    {
                        this->enemyShips[i]->changeMovement(EnemyShip::MovementType::BEZIER);
                    }
                }
                this->enemyBonusIndexes.erase(this->enemyBonusIndexes.begin());
            }
        }

        // display sprites
        window.clear();

        // draw background;
        this->backgroundTexturePositionFloat.y += ((float)this->backgroundVelocity.y * dt);
        this->backgroundTexturePosition.y = (int)this->backgroundTexturePositionFloat.y;
        if (this->backgroundTexturePosition.y > this->backgroundSpriteSize)
        {
            this->backgroundTexturePosition = { 0,0 };
            this->backgroundTexturePositionFloat = { 0.0f, 0.0f };
        }
        this->backgroundSprite.setTextureRect(sf::IntRect(this->backgroundTexturePosition, this->backgroundTextureSize));
        window.draw(this->backgroundSprite);

        this->backgroundTexturePositionFloat2.y += ((float)this->backgroundVelocity.y * dt * 3);
        this->backgroundTexturePosition2.y = (int)this->backgroundTexturePositionFloat2.y;
        if (this->backgroundTexturePosition2.y > this->backgroundSpriteSize)
        {
            this->backgroundTexturePosition2 = { 64,0 };
            this->backgroundTexturePositionFloat2 = { 64.0f, 0.0f };
        }
        this->backgroundSprite2.setTextureRect(sf::IntRect(this->backgroundTexturePosition2, this->backgroundTextureSize));
        window.draw(this->backgroundSprite2);

        for (int i = 0; i < this->backgroundStars.size(); i++)
        {
            this->backgroundStars[i].y -= dt * this->backgroundStarsSpeed;
            if (this->backgroundStars[i].y < this->miny)
            {
                this->backgroundStars[i].y = this->maxy;
                this->backgroundStars[i].x = this->minx + std::rand() % (int)(this->maxx - this->minx);
            }
        }
        sf::VertexArray stars = createVertexArray(this->backgroundStars, sf::Color::White);
        stars.setPrimitiveType(sf::PrimitiveType::Points);
        window.draw(stars);


        // draw game entities

        // debug stuff
        if (this->debugEnabled)
        {
            window.draw(this->box);
            std::string s{ "Enemies: " };
            s.append(std::to_string(this->enemyShips.size()));
            this->textEnemies.setString(s);
            window.draw(this->textEnemies);
        }

        // score
        std::string s{ "Score: " };
        this->textScore.setString(s.append(std::to_string(this->score)));
        window.draw(this->textScore);

        // game assets
        for (int i = 0; i < this->enemyShips.size(); i++)
        {
            window.draw(*this->enemyShips[i]);
        }
        for (int i = 0; i < this->animations.size(); i++)
        {
            window.draw(this->animations[i]);
        }
        for (int i = 0; i < this->playerLasers.size(); i++)
        {
            window.draw(*this->playerLasers[i]);
        }
        for (int i = 0; i < this->playerMissiles.size(); i++)
        {
            window.draw(this->playerMissiles[i]);
        }
        for (int i = 0; i < this->enemyLasers.size(); i++)
        {
            window.draw(*this->enemyLasers[i]);
        }
        for (int i = 0; i < this->powerups.size(); i++)
        {
            window.draw(this->powerups[i]);
        }

        if (this->bossActive)
        {
            sf::VertexArray healthBarOutlineVA;
            healthBarOutlineVA.setPrimitiveType(sf::PrimitiveType::LinesStrip);
            healthBarOutlineVA.append({{ this->minx, this->miny }, sf::Color::Green});
            healthBarOutlineVA.append({{ this->maxx, this->miny }, sf::Color::Green});
            healthBarOutlineVA.append({{ this->maxx, 5.0f }, sf::Color::Green});
            healthBarOutlineVA.append({{ this->minx, 5.0f }, sf::Color::Green});
            healthBarOutlineVA.append({{ this->minx, this->miny }, sf::Color::Green});

            sf::VertexArray healthBarVA;
            healthBarVA.setPrimitiveType(sf::PrimitiveType::TrianglesStrip);
            healthBarVA.append({ { this->minx, this->miny }, sf::Color::Green });
            healthBarVA.append({ { this->minx + (this->maxx - this->minx) * this->enemyShips[0]->hp / 2000, this->miny }, sf::Color::Green });
            healthBarVA.append({ { this->minx + (this->maxx - this->minx) * this->enemyShips[0]->hp / 2000, 5.0f }, sf::Color::Green });
            healthBarVA.append({ { this->minx, 5.0f }, sf::Color::Green });
            healthBarVA.append({ { this->minx, this->miny }, sf::Color::Green });

            window.draw(healthBarOutlineVA);
            window.draw(healthBarVA);
        }

        window.draw(this->playerShip);
    }

};

// declarations
std::unique_ptr<Game> gameState;
MenuEntity menuState;

int main()
{
    std::srand(std::time(nullptr));
    sf::RenderWindow window(sf::VideoMode(1600, 800), "Invaders! Oh noes!");// , sf::Style::Fullscreen);
    sf::View camera;
    camera.setCenter(800, 400);
    camera.setSize(1600, 800);
    window.setView(camera);

    gameState = std::make_unique<Game>();
    //std::unique_ptr<Game> gameState2;
    //gameState2 = gameState; // error
    // music
    sf::Music menuMusic;
    menuMusic.openFromFile("./assets/sound/Common Fight.ogg");
    menuMusic.setLoop(true);
    menuMusic.play();
    menuMusic.setVolume(10.0f);

    globalTextures.init();
    gameState->game_init();
    menuState.menu_init();

    // setting up utility vars

    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosWorld = window.mapPixelToCoords(mousePos);

    sf::Clock frameClock;
    float dt;

    // game loop

    while (window.isOpen())
    {
        bool shouldExit = false;
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
            {
                shouldExit = true;
                break;
            }
            case sf::Event::KeyPressed:
            {
                if (navigation.currentState == Navigation::NavigationStates::GAME_OVER || navigation.currentState == Navigation::NavigationStates::VICTORY)
                {
                    menuState.keyPressed = true;
                }
                break;
            }
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
        {
            shouldExit = true;
        }

        if (shouldExit)
        {
            gameState = nullptr;
            window.close();
            return 0;
        }
        dt = frameClock.restart().asSeconds();
        mousePos = sf::Mouse::getPosition(window);
        mousePosWorld = window.mapPixelToCoords(mousePos);

        switch (navigation.currentState)
        {
		case Navigation::NavigationStates::GAME:
		{
			if (navigation.gameOver)
			{
				gameState->game_init();
				navigation.gameOver = false;
			}
			gameState->game_loop(dt, window);
			break;
		}
		case Navigation::NavigationStates::MENU:
		{
			menuState.menu_loop(dt, window);
			break;
		}
		case Navigation::NavigationStates::GAME_OVER:
		{
			menuState.game_over_loop(dt, window);
			break;
		}
		case Navigation::NavigationStates::VICTORY:
		{
			menuState.victory_loop(dt, window);
			break;
		}
        }
        window.display();

    }
    return 0;
}