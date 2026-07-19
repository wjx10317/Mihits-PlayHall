-- 外部游戏专区资源元数据（PlayHallServer）
-- 全量更新 = manifest diff 为全集；仅一个 manifest_url

CREATE TABLE IF NOT EXISTS t_game_pkg (
  zoneid        INT          NOT NULL PRIMARY KEY COMMENT '与专区 ID 相同',
  version       VARCHAR(32)  NOT NULL COMMENT '当前版本号',
  exe_name      VARCHAR(64)  NOT NULL COMMENT '可执行文件名(无扩展名)，即安装目录名',
  manifest_url  VARCHAR(512) NOT NULL COMMENT '清单唯一入口 URL',
  release_note  VARCHAR(256) NOT NULL DEFAULT '',
  updated_at    TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
                             ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 示例行（按内网 Nginx 地址修改后插入）
-- zoneid=0x20(32) 避开五子棋 0x10
INSERT INTO t_game_pkg (zoneid, version, exe_name, manifest_url, release_note)
VALUES (
  32,
  '1.0.0',
  'FooGame',
  'http://192.168.1.50/games/FooGame/manifest.json',
  'external game demo'
) ON DUPLICATE KEY UPDATE
  version = VALUES(version),
  exe_name = VALUES(exe_name),
  manifest_url = VALUES(manifest_url),
  release_note = VALUES(release_note);
