#!/bin/bash

WIDTH=40
HEIGHT=20
HIGHSCORE_FILE="/home/ntust/Ivan/minigame/highscores.txt"

init_colors() {
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    BLUE='\033[0;34m'
    YELLOW='\033[1;33m'
    CYAN='\033[0;36m'
    MAGENTA='\033[0;35m'
    WHITE='\033[1;37m'
    GRAY='\033[0;90m'
    NC='\033[0m'
    BOLD='\033[1m'
}

init_colors

draw_title() {
    clear
    echo -e "${CYAN}"
    cat << 'EOF'
              ████████████                                   
          ████            ████                               
       ██                    ██                             
     ██    ████████████████    ██                           
    ██   ██                ██   ██                          
   ██   ██    ████████      ██   ██                         
   ██  ██    ██      ██     ██   ██                         
   ██  ██   ██        ██   ██    ██                        
    ██  ██              ██  ██    ██                       
     ██   ████████████████   ██                           
       ██                    ██                            
         ████████████████████                               
                                                                    
    ██████  ██████  ██████  ██████  ██████  ██████           
   ██      ██  ██  ██  ██    ██    ██  ██  ██              
   ██      ██  ██  ██  ██    ██    ██  ██  ██              
    ██████ ██████  ██████    ██    ██████  ██████          
                                                                    
              ╔═══════════════════════════════╗              
              ║     貪吃蛇：烈空座進化         ║              
              ║   Snake Evolution: Rayquaza    ║              
              ╚═══════════════════════════════╝              
EOF
    echo -e "${NC}"
    echo -e "${YELLOW}控制說明:${NC}"
    echo -e "  ${GREEN}W/↑${NC} - 上移  ${GREEN}S/↓${NC} - 下移  ${GREEN}A/←${NC} - 左移  ${GREEN}D/→${NC} - 右移"
    echo -e "  ${MAGENTA}SPACE${NC} - 龍波動 (清除敵人) [烈空座]"
    echo -e "  ${RED}Q${NC} - 暴風驅散 (無敵3秒) [烈空座]"
    echo -e "  ${GRAY}P${NC} - 暫停  ${GRAY}ESC${NC} - 結束"
    echo ""
    echo -e "${YELLOW}進化系統:${NC}"
    echo -e "  ${GREEN}蛇 (Snake)${NC}: 0-9 分  - 基本能力"
    echo -e "  ${BLUE}龍 (Dragon)${NC}: 10-24 分 - 穿透敵人"
    echo -e "  ${CYAN}烈空座 (Rayquaza)${NC}: 25+ 分 - 穿牆 + 技能"
    echo ""
    echo -e "${YELLOW}困難設計:${NC}"
    echo -e "  每 3 秒生成障礙物"
    echo -e "  每 5 秒生成敵人"
    echo ""
    read -n 1 -s -p "  按任意鍵開始遊戲..."
}

init_game() {
    snake_x=($((WIDTH/2)) $((WIDTH/2-1)) $((WIDTH/2-2)))
    snake_y=(10 10 10)
    snake_len=3
    direction="RIGHT"
    score=0
    evolution_stage=0
    speed=150000
    game_over=0
    paused=0
    invincible=0
    invincible_time=0
    dragon_dance_ready=1
    wall_pass_ready=1
    
    food_x=0
    food_y=0
    spawn_food
    
    obstacles=()
    enemies=()
    
    enemy_count=0
    obstacle_count=0
    
    game_start_time=$(date +%s)
    last_obstacle_time=$game_start_time
    last_enemy_time=$game_start_time
    
    tput civis
    trap '' SIGINT
}

spawn_food() {
    while true; do
        food_x=$((RANDOM % (WIDTH-2) + 1))
        food_y=$((RANDOM % (HEIGHT-2) + 1))
        
        collision=0
        for ((i=0; i<snake_len; i++)); do
            if [[ ${snake_x[$i]} -eq $food_x && ${snake_y[$i]} -eq $food_y ]]; then
                collision=1
                break
            fi
        done
        
        if [[ $collision -eq 0 ]]; then
            for obs in "${obstacles[@]}"; do
                IFS=',' read -r ox oy <<< "$obs"
                if [[ $ox -eq $food_x && $oy -eq $food_y ]]; then
                    collision=1
                    break
                fi
            done
        fi
        
        [[ $collision -eq 0 ]] && break
    done
}

spawn_obstacle() {
    while true; do
        ox=$((RANDOM % (WIDTH-2) + 1))
        oy=$((RANDOM % (HEIGHT-2) + 1))
        
        if [[ $ox -eq $((WIDTH/2)) && $oy -eq 10 ]]; then
            continue
        fi
        
        collision=0
        for ((i=0; i<snake_len; i++)); do
            if [[ ${snake_x[$i]} -eq $ox && ${snake_y[$i]} -eq $oy ]]; then
                collision=1
                break
            fi
        done
        
        [[ $collision -eq 0 ]] && break
    done
    obstacles+=("$ox,$oy")
}

spawn_enemy() {
    while true; do
        ex=$((RANDOM % (WIDTH-2) + 1))
        ey=$((RANDOM % (HEIGHT-2) + 1))
        
        collision=0
        for ((i=0; i<snake_len; i++)); do
            if [[ ${snake_x[$i]} -eq $ex && ${snake_y[$i]} -eq $ey ]]; then
                collision=1
                break
            fi
        done
        
        for obs in "${obstacles[@]}"; do
            IFS=',' read -r ox oy <<< "$obs"
            if [[ $ox -eq $ex && $oy -eq $ey ]]; then
                collision=1
                break
            fi
        done
        
        [[ $collision -eq 0 ]] && break
    done
    enemies+=("$ex,$ey,0")
}

move_enemies() {
    new_enemies=()
    for enemy in "${enemies[@]}"; do
        IFS=',' read -r ex ey etype <<< "$enemy"
        
        dx=0
        dy=0
        
        if [[ $ex -lt ${snake_x[0]} ]]; then
            dx=1
        elif [[ $ex -gt ${snake_x[0]} ]]; then
            dx=-1
        fi
        
        if [[ $ey -lt ${snake_y[0]} ]]; then
            dy=1
        elif [[ $ey -gt ${snake_y[0]} ]]; then
            dy=-1
        fi
        
        if [[ $((RANDOM % 2)) -eq 0 && $dx -ne 0 ]]; then
            dy=0
        elif $dy -ne 0 ]; then
            dx=0
        fi
        
        if [[ $evolution_stage -ge 1 ]]; then
            ((ex+=dx))
            ((ey+=dy))
        else
            if [[ $((RANDOM % 3)) -ne 0 ]]; then
                ((ex+=dx))
                ((ey+=dy))
            fi
        fi
        
        [[ $ex -ge 0 && $ex -lt $WIDTH && $ey -ge 0 && $ey -lt $HEIGHT ]] || continue
        
        collision=0
        for obs in "${obstacles[@]}"; do
            IFS=',' read -r ox oy <<< "$obs"
            if [[ $ox -eq $ex && $oy -eq $ey ]]; then
                collision=1
                break
            fi
        done
        
        [[ $collision -eq 0 ]] && new_enemies+=("$ex,$ey,$etype")
    done
    enemies=("${new_enemies[@]}")
}

check_collisions() {
    head_x=${snake_x[0]}
    head_y=${snake_y[0]}
    
    if [[ $evolution_stage -lt 2 || $wall_pass_ready -eq 0 ]]; then
        if [[ $head_x -le 0 || $head_x -ge $((WIDTH-1)) || $head_y -le 0 || $head_y -ge $((HEIGHT-1)) ]]; then
            if [[ $evolution_stage -eq 2 && $wall_pass_ready -eq 1 ]]; then
                if [[ $head_x -le 0 ]]; then
                    snake_x[0]=$((WIDTH-2))
                elif [[ $head_x -ge $((WIDTH-1)) ]]; then
                    snake_x[0]=1
                elif [[ $head_y -le 0 ]]; then
                    snake_y[0]=$((HEIGHT-2))
                elif [[ $head_y -ge $((HEIGHT-1)) ]]; then
                    snake_y[0]=1
                fi
                wall_pass_ready=0
            else
                game_over=1
                return
            fi
        fi
    else
        if [[ $head_x -le 0 ]]; then
            snake_x[0]=$((WIDTH-2))
        elif [[ $head_x -ge $((WIDTH-1)) ]]; then
            snake_x[0]=1
        elif [[ $head_y -le 0 ]]; then
            snake_y[0]=$((HEIGHT-2))
        elif [[ $head_y -ge $((HEIGHT-1)) ]]; then
            snake_y[0]=1
        fi
    fi
    
    for ((i=1; i<snake_len; i++)); do
        if [[ ${snake_x[$i]} -eq $head_x && ${snake_y[$i]} -eq $head_y ]]; then
            game_over=1
            return
        fi
    done
    
    for obs in "${obstacles[@]}"; do
        IFS=',' read -r ox oy <<< "$obs"
        if [[ $ox -eq $head_x && $oy -eq $head_y ]]; then
            if [[ $evolution_stage -ge 1 ]]; then
                new_obstacles=()
                for o in "${obstacles[@]}"; do
                    if [[ "$o" != "$obs" ]]; then
                        new_obstacles+=("$o")
                    fi
                done
                obstacles=("${new_obstacles[@]}")
                score=$((score + 2))
                
                if [[ $score -eq 10 && $evolution_stage -eq 0 ]]; then
                    evolution_stage=1
                    speed=130000
                    draw_evolution_animation "龍 (DRAGON)"
                elif [[ $score -eq 25 && $evolution_stage -eq 1 ]]; then
                    evolution_stage=2
                    speed=100000
                    draw_evolution_animation "烈空座 (RAYQUAZA)"
                fi
            else
                game_over=1
                return
            fi
        fi
    done
    
    for enemy in "${enemies[@]}"; do
        IFS=',' read -r ex ey etype <<< "$enemy"
        if [[ $ex -eq $head_x && $ey -eq $head_y ]]; then
            if [[ $invincible -eq 1 ]]; then
                continue
            elif [[ $evolution_stage -ge 1 ]]; then
                new_enemies=()
                for e in "${enemies[@]}"; do
                    if [[ "$e" != "$enemy" ]]; then
                        new_enemies+=("$e")
                    fi
                done
                enemies=("${new_enemies[@]}")
                score=$((score + 3))
                
                if [[ $score -eq 10 && $evolution_stage -eq 0 ]]; then
                    evolution_stage=1
                    speed=130000
                    draw_evolution_animation "龍 (DRAGON)"
                elif [[ $score -eq 25 && $evolution_stage -eq 1 ]]; then
                    evolution_stage=2
                    speed=100000
                    draw_evolution_animation "烈空座 (RAYQUAZA)"
                fi
            else
                game_over=1
                return
            fi
        fi
    done
    
    if [[ $head_x -eq $food_x && $head_y -eq $food_y ]]; then
        score=$((score + 1))
        
        if [[ $score -eq 10 && $evolution_stage -eq 0 ]]; then
            evolution_stage=1
            speed=130000
            draw_evolution_animation "龍 (DRAGON)"
        elif [[ $score -eq 25 && $evolution_stage -eq 1 ]]; then
            evolution_stage=2
            speed=100000
            draw_evolution_animation "烈空座 (RAYQUAZA)"
        fi
        
        if [[ $((score % 3)) -eq 0 && $score -gt 0 ]]; then
            speed=$((speed - 3000))
            if [[ $speed -lt 50000 ]]; then
                speed=50000
            fi
        fi
        
        snake_len=$((snake_len + 1))
        spawn_food
    fi
}

draw_evolution_animation() {
    local evo_name="$1"
    for i in {1..5}; do
        tput cup 0 0
        echo -e "${YELLOW}╔════════════════════════════════════════╗${NC}"
        echo -e "${YELLOW}║         進化中... $evo_name         ║${NC}"
        echo -e "${YELLOW}╚════════════════════════════════════════╝${NC}"
        sleep 0.1
        tput cup 0 0
        echo -e "${CYAN}╔════════════════════════════════════════╗${NC}"
        echo -e "${CYAN}║         進化中... $evo_name         ║${NC}"
        echo -e "${CYAN}╚════════════════════════════════════════╝${NC}"
        sleep 0.1
    done
}

move_snake() {
    new_head_x=${snake_x[0]}
    new_head_y=${snake_y[0]}
    
    case $direction in
        UP)    ((new_head_y--));;
        DOWN)  ((new_head_y++));;
        LEFT)  ((new_head_x--));;
        RIGHT) ((new_head_x++));;
    esac
    
    for ((i=$((snake_len-1)); i>0; i--)); do
        snake_x[$i]=${snake_x[$((i-1))]}
        snake_y[$i]=${snake_y[$((i-1))]}
    done
    
    snake_x[0]=$new_head_x
    snake_y[0]=$new_head_y
}

draw_game() {
    tput cup 0 0
    
    echo -e "${GRAY}┌${GRAY}──────────────────────────────────────${GRAY}┐${NC}"
    
    for ((y=1; y<HEIGHT-1; y++)); do
        echo -ne "${GRAY}│${NC}"
        for ((x=1; x<WIDTH-1; x++)); do
            char=" "
            color="${NC}"
            
            if [[ $x -eq $food_x && $y -eq $food_y ]]; then
                if [[ $evolution_stage -eq 2 ]]; then
                    char="★"
                    color="${YELLOW}"
                elif [[ $evolution_stage -eq 1 ]]; then
                    char="◆"
                    color="${BLUE}"
                else
                    char="●"
                    color="${GREEN}"
                fi
            fi
            
            for obs in "${obstacles[@]}"; do
                IFS=',' read -r ox oy <<< "$obs"
                if [[ $ox -eq $x && $oy -eq $y ]]; then
                    char="█"
                    color="${GRAY}"
                    break
                fi
            done
            
            for enemy in "${enemies[@]}"; do
                IFS=',' read -r ex ey <<< "$enemy"
                if [[ $ex -eq $x && $ey -eq $y ]]; then
                    char="✸"
                    color="${RED}"
                    break
                fi
            done
            
            for ((i=0; i<snake_len; i++)); do
                if [[ ${snake_x[$i]} -eq $x && ${snake_y[$i]} -eq $y ]]; then
                    if [[ $i -eq 0 ]]; then
                        if [[ $evolution_stage -eq 2 ]]; then
                            char="⚡"
                            color="${CYAN}"
                        elif [[ $evolution_stage -eq 1 ]]; then
                            char="◎"
                            color="${BLUE}"
                        else
                            char="●"
                            color="${GREEN}"
                        fi
                    else
                        if [[ $evolution_stage -eq 2 ]]; then
                            char="▼"
                            color="${CYAN}"
                        elif [[ $evolution_stage -eq 1 ]]; then
                            char="○"
                            color="${BLUE}"
                        else
                            char="○"
                            color="${GREEN}"
                        fi
                    fi
                    break
                fi
            done
            
            echo -ne "${color}${char}${NC}"
        done
        echo -e "${GRAY}│${NC}"
    done
    
    echo -e "${GRAY}└──────────────────────────────────────┘${NC}"
    
    if [[ $evolution_stage -eq 0 ]]; then
        stage_name="${GREEN}蛇${NC}"
    elif [[ $evolution_stage -eq 1 ]]; then
        stage_name="${BLUE}龍${NC}"
    else
        stage_name="${CYAN}烈空座${NC}"
    fi
    
    echo -ne "${BOLD}分數: ${score}${NC} | 形態: ${stage_name}"
    
    if [[ $evolution_stage -eq 2 ]]; then
        echo -ne " | ${MAGENTA}龍波動: ${dragon_dance_ready}${NC}"
        echo -ne " | ${RED}暴風: ${invincible}${NC}"
    fi
    
    echo ""
}

handle_input() {
    if read -t 0.001 -n 1 key; then
        case $key in
            w|W|8)
                [[ $direction != "DOWN" ]] && direction="UP"
                ;;
            s|S|2)
                [[ $direction != "UP" ]] && direction="DOWN"
                ;;
            a|A|4)
                [[ $direction != "RIGHT" ]] && direction="LEFT"
                ;;
            d|D|6)
                [[ $direction != "LEFT" ]] && direction="RIGHT"
                ;;
            p|P)
                paused=$((1-paused))
                ;;
            $'\e')
                read -t 0.001 -n 1 key
                case $key in
                    A) [[ $direction != "DOWN" ]] && direction="UP" ;;
                    B) [[ $direction != "UP" ]] && direction="DOWN" ;;
                    C) [[ $direction != "LEFT" ]] && direction="RIGHT" ;;
                    D) [[ $direction != "RIGHT" ]] && direction="LEFT" ;;
                    *) game_over=2 ;;
                esac
                ;;
            ' ')
                if [[ $evolution_stage -eq 2 && $dragon_dance_ready -eq 1 ]]; then
                    dragon_dance_ready=0
                    enemies=()
                    score=$((score + 5))
                    sleep 3
                    dragon_dance_ready=1
                fi
                ;;
            q|Q)
                if [[ $evolution_stage -eq 2 && $invincible -eq 0 ]]; then
                    invincible=1
                    invincible_time=3
                    while [[ $invincible_time -gt 0 ]]; do
                        sleep 1
                        ((invincible_time--))
                    done
                    invincible=0
                fi
                ;;
        esac
    fi
}

save_score() {
    mkdir -p "$(dirname "$HIGHSCORE_FILE")"
    
    if [[ ! -f "$HIGHSCORE_FILE" ]]; then
        echo "0 匿名" > "$HIGHSCORE_FILE"
        echo "0 匿名" >> "$HIGHSCORE_FILE"
        echo "0 匿名" >> "$HIGHSCORE_FILE"
    fi
    
    name=$(whoami)
    
    scores=()
    names=()
    
    while IFS=' ' read -r sc nm; do
        scores+=("$sc")
        names+=("$nm")
    done < "$HIGHSCORE_FILE"
    
    new_scores=()
    new_names=()
    inserted=0
    
    for i in "${!scores[@]}"; do
        if [[ $inserted -eq 0 && $score -gt ${scores[$i]} ]]; then
            new_scores+=("$score")
            new_names+=("$name")
            inserted=1
        fi
        if [[ ${#new_scores[@]} -lt 5 ]]; then
            new_scores+=("${scores[$i]}")
            new_names+=("${names[$i]}")
        fi
    done
    
    if [[ $inserted -eq 0 && ${#new_scores[@]} -lt 5 ]]; then
        new_scores+=("$score")
        new_names+=("$name")
    fi
    
    {
        for i in "${!new_scores[@]}"; do
            echo "${new_scores[$i]} ${new_names[$i]}"
        done
    } > "$HIGHSCORE_FILE"
}

show_leaderboard() {
    clear
    echo -e "${CYAN}"
    echo "╔═══════════════════════════════╗"
    echo "║        🏆 排行榜 🏆           ║"
    echo "╚═══════════════════════════════╝"
    echo -e "${NC}"
    
    if [[ -f "$HIGHSCORE_FILE" ]]; then
        rank=1
        while IFS=' ' read -r sc nm; do
            if [[ $sc -gt 0 ]]; then
                if [[ $rank -eq 1 ]]; then
                    echo -e "  ${YELLOW}🥇 ${rank}. ${nm}: ${sc} 分${NC}"
                elif [[ $rank -eq 2 ]]; then
                    echo -e "  ${GRAY}🥈 ${rank}. ${nm}: ${sc} 分${NC}"
                elif [[ $rank -eq 3 ]]; then
                    echo -e "  ${MAGENTA}🥉 ${rank}. ${nm}: ${sc} 分${NC}"
                else
                    echo -e "  ${rank}. ${nm}: ${sc} 分"
                fi
                ((rank++))
            fi
        done < "$HIGHSCORE_FILE"
    else
        echo "  還沒有記錄!"
    fi
    
    echo ""
    read -n 1 -s -p "  按任意鍵返回..."
}

show_game_over() {
    tput cnorm
    trap SIGINT
    
    if [[ $game_over -eq 1 ]]; then
        echo -e "\n${RED}"
        echo "  ██████   █████  ███    ███ ███████      ██████  ██    ██ ███████ ██████  "
        echo " ██       ██   ██ ████  ████ ██          ██    ██ ██    ██ ██      ██   ██ "
        echo " ██   ███ ███████ ██ ████ ██ █████       ██    ██ ██    ██ █████   ██████  "
        echo " ██    ██ ██   ██ ██  ██  ██ ██          ██    ██  ██  ██  ██      ██   ██ "
        echo "  ██████  ██   ██ ██      ██ ███████      ██████    ████   ███████ ██   ██ "
        echo -e "${NC}"
    fi
    
    echo -e "\n  ${BOLD}最終分數: ${score}${NC}"
    echo -e "  最終形態: $stage_name"
    echo ""
    
    save_score
    
    echo -ne "${CYAN}  [1]${NC} 查看排行榜  "
    echo -e "${CYAN}[2]${NC} 再來一局  "
    echo -e "${CYAN}[3]${NC} 結束遊戲"
    echo ""
    echo -ne "  選擇: "
    read -n 1 choice
    
    case $choice in
        1) show_leaderboard; main_menu ;;
        2) game_loop ;;
        3) exit 0 ;;
        *) exit 0 ;;
    esac
}

main_menu() {
    draw_title
    game_loop
}

game_loop() {
    init_game
    
    while [[ $game_over -eq 0 ]]; do
        if [[ $paused -eq 1 ]]; then
            tput cup $((HEIGHT/2)) $((WIDTH/2-5))
            echo -e "${YELLOW}暫停中...${NC}"
            sleep 0.5
            continue
        fi
        
        handle_input
        
        if [[ $game_over -ne 0 ]]; then
            break
        fi
        
        move_snake
        check_collisions
        
        if [[ $game_over -ne 0 ]]; then
            break
        fi
        
        if [[ $enemy_count -gt 0 ]]; then
            move_enemies
            check_collisions
        fi
        
        if [[ $game_over -ne 0 ]]; then
            break
        fi
        
        draw_game
        
        current_time=$(date +%s)
        elapsed=$((current_time - game_start_time))
        
        if [[ $((current_time - last_obstacle_time)) -ge 3 ]]; then
            spawn_obstacle
            ((obstacle_count++))
            last_obstacle_time=$current_time
        fi
        
        if [[ $((current_time - last_enemy_time)) -ge 5 ]]; then
            spawn_enemy
            ((enemy_count++))
            last_enemy_time=$current_time
        fi
        
        if [[ $wall_pass_ready -eq 0 && $evolution_stage -eq 2 ]]; then
            sleep 5
            wall_pass_ready=1
        fi
        
        usleep $speed
    done
    
    show_game_over
}

main_menu
