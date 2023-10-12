# 2023_RoboTracer
## 1.概要
　この機体は，ロボトレース競技に参加するためにサークル標準機を改良したロボットです．ロボトレを始めてから1年が経過したため，今までの改良をまとめようと思います．

　このリポジトリは，改良内容の言語化による問題点の明確化，後輩に改良の一例として提示したいという目的で作成しました．そのため，
 
## 2.ディレクトリ構造

## 3.開発環境
 - Visual Studio Code

 - Fusion360

 - Eagle

## 4.機体説明
<p align="center">
 <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/6678fec6-45e2-4e66-b0d3-dd4fd7b67914" width="400px">
</p>

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

    
## 5.改良点
- ラインセンサ

  　Raspberry Pi Picoには，ユーザが使えるADCが3つしかありません．そのため，12bitの外付けADコンバータによるADCの増設をしました．ラズピコの2チャンネル+ADコンバータの8チャンネルを使用し，計10個のラインセンサを使えるようにしました．
<p align="center">
 <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/81f3ebca-04ac-492f-b433-5287070fb427" width="400px">
</p>



- モータref電圧の上昇

  　ステッピングモータは電圧をかけることでトルクと最高速度が上がるため、モタドラへのref電圧を上げました。他の手法としてモータへかける電圧を上げる　　が挙げられますが、コンデンサの交換等が必要となるため、改良が容易なref電圧を上げる選択をしました。
  

- IMUの搭載
  
    　2次走行を行うにはロボットの角度を取得する必要があるため，ジャイロセンサを搭載しました．偶然手持ちにあったMPU6050を使用しています．
  
  <p align="center">
    <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/0492ced1-3b90-49bc-9984-d6cbd6b88eaa" width="400px">
  </p>




  走行中の角速度をグラフにすると，以下のようになります．

  <p align="center">
   <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/624c51fc-5e0c-4b81-ba9d-a0cc6703ef79" width="700px">
  </p>

  
  ある範囲外の値を取得できていないことが分かります．これはジャイロのフルスケールレンジが関係しています．MPU6050の初期フルスケールレンジは±250°となっているため，以下のようにして±2000°に設定します．

  ```Swift
  #define MPU6050_ADDRESS 0x68
  
  uint8_t leg[] = {0x1B, 0x18};
  i2c_write_blocking(gyro_i2c, MPU6050_ADDRESS,leg, sizeof(leg), false );
  ```



  改めてグラフを作ると，値を正しく取得できている事が分かります．
 
    <p align="center">
     <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/64335dca-c771-4c69-a1c2-abb5698ec233" width="700px">
    </p>
  
- CPU層

   以上の改良に対応できる基盤を新たに製作した．


- シャーシの製作

    センサ、などをまとめれるシャーシを3dプリンタで製作した。

- タイヤ幅
  標準機のタイヤ幅8mmは、重量700gに対して薄すぎたため、180°のR10でスリップしていた。
  
## 6.制御方法

PD制御を用いてラインへの追従をしています．PD制御を行うには以下のように偏差eを求める必要があります．

<img src="https://latex.codecogs.com/svg.image?\small&space;&space;e=(L5k5&plus;L4k4&plus;L3k3&plus;L2k5&plus;L2k2&plus;L1k1)-(R5k5&plus;R4k4&plus;R3k3&plus;R2k5&plus;R2k2&plus;R1k1)">
ラインセンサのアナログ値を左から L5，L4，L3，L2，L1，R1，R2…R5 とし，セン
サのアナログ値には距離が離れたセンサほど重みをつけるため，定数 k1～k5 をかけている．

 ```Swift
  encoder = TIMX -> CNT;
  encoder = (encoder - 32767);
  
  speed = ((297 * M_PI) / 1146.9) * encoder;
  
  TIMX -> CNT = 32767;
  ```


## 7.ゴール判断

　単純にゴールセンサが反応した時にゴール判断をしてしまうと，クロスでもゴールセンサが反応してしまいます．そのため，クロスではゴール判断をしないという処理が必要となります．ゴールセンサがクロスに反応する前にラインセンサがクロスにかかるので，ライセンサを使ってクロスの判断ができそうです．両端のラインセンサが反応した時にフラグを立て，フラグが立っている時にゴールセンサが反応したら無視すればクロス問題は解決できそうです．

 
## 8.コースの記憶
　加減速走行を行うには，再現性のある走行とコースの記憶が必要となります．そのため走行経路を2次元座標にプロットし，再現性の確認とコースの記憶をできるようにしました．加減速走行をするだけなら，区間距離と角速度情報があればわざわざコースのプロットをする必要がありません．しかし，まだ先の話ですがショートカット走行を行う際に役立つと考えたので先行開発しました．
 　
　
## 8.加減速走行

[参考にさせていただいたサイト](https://github.com/raspberrypi/pico-examples/blob/master/i2c/mpu6050_i2c/mpu6050_i2c.c)

[参考にさせていただいたサイト](https://qiita.com/jamjam/items/6d49f9200d809b4a1d72)
