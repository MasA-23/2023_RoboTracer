# 2023_RoboTracer
## 1.概要
この機体は，ロボトレース競技に参加するためにサークル標準機を改良したロボットです．ロボトレを始めてから1年が経過したため，今までの改良をまとめようと思います．

このリポジトリは，改良内容の言語化による問題点の明確化，後輩に改良の一例として提示したいという目的で作成しました．そのため，

## 2.機体説明
<img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/5d4b12e6-783b-444a-a7d2-bf73ace43d1b" width="400px">


- 機体名 : Savoia S.21M


- スペック
  - L×W×H[mm] : 155x111x80
  - 重量[g] : 700
  - タイヤ径[mm] : 52
  - 最高速度[m/s] : 2.5
  - 最高加速度[m/s^2] : 


- 構成要素
  - マイコンボード : Raspberry Pi Pico
  - アクチュエータ : PKE243A-L
  - IMU : MPU-6050
  - ラインセンサ : LBR-127HLD
  - ADコンバータ : MCP3208
    
## 3.改良点
- ラインセンサ
  
Raspberry Pi Picoにはユーザが使えるADCが3つしかありません．そのため，ADコンバータによるADCの増設を行いました。(適当に買ったやつがたまたま12bitだった…ｱﾌﾞﾈ)

<img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/81f3ebca-04ac-492f-b433-5287070fb427" width="400px">

## 3.制御方法


 ```Swift
  encoder = TIMX -> CNT;
  encoder = (encoder - 32767);
  
  speed = ((297 * M_PI) / 1146.9) * encoder;
  
  TIMX -> CNT = 32767;
  ```
